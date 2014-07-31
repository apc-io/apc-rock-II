#include <linux/unistd.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
//#include <asm/semaphore.h>
#include <linux/proc_fs.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <linux/i2c.h>
#include <linux/irq.h>

#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/syscalls.h>

#include "gslX680.h"
#include "wmt_ts.h"

/////////////////////////////////////////////////////////////////

// commands for ui
#define TS_IOC_MAGIC  't'

#define TS_IOCTL_CAL_START    _IO(TS_IOC_MAGIC,   1)
#define TS_IOCTL_CAL_DONE     _IOW(TS_IOC_MAGIC,  2, int*)
#define TS_IOCTL_GET_RAWDATA  _IOR(TS_IOC_MAGIC,  3, int*)
#define TS_IOCTL_CAL_QUIT	_IOW(TS_IOC_MAGIC,  4, int*)
#define TS_IOCTL_AUTO_CALIBRATION	_IOW(TS_IOC_MAGIC,  5, int*)
#define TS_IOC_MAXNR          5

//
#define TS_MAJOR                	11
#define TS_DRIVER_NAME	 	"wmtts_touch"
#define TS_NAME          	 	"wmtts"
#define WMTTS_PROC_NAME     "wmtts_config"

#define EXT_GPIO0   	0
#define EXT_GPIO1   	1
#define EXT_GPIO2   	2
#define EXT_GPIO3   	3
#define EXT_GPIO4	4
#define EXT_GPIO5	5
#define EXT_GPIO6  	6
#define EXT_GPIO7   	7

typedef struct {
    int   a1;
    int   b1;
    int   c1;
    int   a2;
    int   b2;
    int   c2;
    int   delta;
}CALIBRATION_PARAMETER, *PCALIBRATION_PARAMETER;


static int irq_gpio;
static int rst_gpio;
static int panelres_x;
static int panelres_y;
static DECLARE_WAIT_QUEUE_HEAD(queue);
static CALIBRATION_PARAMETER g_CalcParam;
static TS_EVENT g_evLast;
static struct mutex cal_mutex;
static DECLARE_WAIT_QUEUE_HEAD(ts_penup_wait_queue);

static struct class* l_dev_class = NULL;
static struct device *l_clsdevice = NULL;
extern struct wmtts_device gslx680_tsdev;
static struct wmtts_device* l_tsdev = &gslx680_tsdev;
struct proc_dir_entry* l_tsproc = NULL;
static struct i2c_client *l_client=NULL;
static int l_penup = 0; // 1-pen up,0-pen down
static int l_iftskey = -1; // 1:have ts key
int earlysus_en = 0;

struct tp_infor
{
	//enum tp_type type;
	char name[64];
	int i2caddr;
	int xaxis; //0: x,1: x swap with y
	int xdir; // 1: positive,-1: revert
	int ydir; // 1: positive,-1: revert
	int finger_num;
};

static int l_tpindex = -1;
static struct tp_infor l_tpinfor[1];


/////////////////////////////////////////////////////
//    function declare
/////////////////////////////////////////////////////
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, unsigned char *varval);
static int ts_writeproc( struct file   *file,
                           const char    *buffer,
                           unsigned long count,
                           void          *data );
static int ts_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data);
///////////////////////////////////////////////////////////////////////
void TouchPanelCalibrateAPoint(
    int   UncalX,     //@PARM The uncalibrated X coordinate
    int   UncalY,     //@PARM The uncalibrated Y coordinate
    int   *pCalX,     //@PARM The calibrated X coordinate
    int   *pCalY      //@PARM The calibrated Y coordinate
    )
{
	int   x, y;
	mutex_lock(&cal_mutex);
    	x = (g_CalcParam.a1 * UncalX + g_CalcParam.b1 * UncalY +
         	g_CalcParam.c1) / g_CalcParam.delta;
    	y = (g_CalcParam.a2 * UncalX + g_CalcParam.b2 * UncalY +
         	g_CalcParam.c2) / g_CalcParam.delta;

//klog("afer(%d,%d)(%d,%d)\n", x,y,panelres_x,panelres_y);
    	if ( x < 0 )
       	 x = 0;

    	if ( y < 0 )
        	y = 0;
        if (x >= panelres_x)
        	x = panelres_x-1;
        if (y >= panelres_y)
        	y = panelres_y-1;

    	*pCalX = x;
    	*pCalY = y;
	mutex_unlock(&cal_mutex);
	return;
}

int wmt_ts_if_tskey(void)
{	
	return ((l_iftskey==1) ? 1: 0);
}

int  wmt_ts_get_firmwfilename(char* fname)
{
	sprintf(fname,"%s_fw.tpf",l_tpinfor[l_tpindex].name);
	return 0;
}


static struct device* get_tp_device(void){
	if(l_client == NULL){
		errlog("l_client is NULL\n");
	}
	return &l_client->dev;
}


unsigned int wmt_ts_get_xaxis(void)
{
	return l_tpinfor[l_tpindex].xaxis;
}

unsigned int wmt_ts_get_xdir(void)
{
	return l_tpinfor[l_tpindex].xdir;
}

unsigned int wmt_ts_get_ydir(void)
{
	return l_tpinfor[l_tpindex].ydir;
}

static int parse_firmwarefile(const char* filedata, struct fw_data** firmarr, unsigned int* id_config)
{
	char endflag[]="/* End flag */";
	const char *p = filedata;	
	u32 val[2];
	int i = 0;
	int j = 0;
	int k = 0;
	const char *s = NULL;

#ifdef	GSL_NOID_VERSION
	if (gsl_noid_ver) {
		while (*p != '{') p++; 
		p++;
		while (*p != '}') {
			sscanf(p,"%u,",&(id_config[k++]));
			p = strchr(p,',');p++;
		}
	}
#endif

	// the first {
	while (*p!='{') p++; 
	p++;
	s = p;
	// calculate the number of array
	while (strncmp(p,endflag,strlen(endflag)))
	{
		if (*p=='{')
		{
			i++;
		}
		p++;
	};
	dbg("the number of arry:0x%x\n", i);
	// alloc the memory for array
	*firmarr = kzalloc(sizeof(struct fw_data)*i, GFP_KERNEL);
	// parse the value of array
	p = s;
	j = 0;
	while (strncmp(p,endflag,strlen(endflag)))
	{
		if (*p=='{')
		{
			memset(val,0,sizeof(val));
			sscanf(p,"{%x,%x}",val,val+1);
			(*firmarr)[j].offset = val[0]&0x00FF;
			(*firmarr)[j].val= val[1];
			//dbg("data%x{%x,%x}\n",j,(*firmarr)[j].offset,(*firmarr)[j].val);
			j++;
		}
		//p = strchr(p,'}');
		p++;
		if (j>=i-2)
		{
			dbg("%s",p);
		}
		
	};
	if (i != j)
	{
		errlog("Error parsing file(the number of arry not match)!\n");
		errlog("i=0x%x,j=0x%x\n", i,j);
		kfree(*firmarr);
		return -1;
	};
	dbg("paring firmware file end.\n");
	return i;
}



//filepath:the path of firmware file;
//firmdata:store the data from firmware file;
//id_config:config data for some ICs whitch have no id;
//return:the lengthen of firmware data,negative-parsing error.
int read_firmwfile(char* filepath, struct fw_data** firmdata, unsigned int* id_config)
{
    const u8 *data = NULL;
    int i = 0;
	int ret = -1;
	const struct firmware* tpfirmware = NULL;

	klog("ts firmware file:%s\n",filepath);
	ret = request_firmware(&tpfirmware, filepath, get_tp_device());
	if (ret < 0) {
		errlog("Failed load tp firmware: %s  ret=%d\n", filepath,ret);
		goto err_end;
	}
	data = tpfirmware->data;
	
    i = parse_firmwarefile(data,firmdata,id_config);
	
    if (i <= 0){
    	dbg("Failed to pare firmware file!\n");
		ret = -1;
    	goto error_parse_fw;
    }
    dbg("firmware arry len=0x%x\n", i);
	ret = i;
    dbg("success to read firmware file!\n");;
error_parse_fw:
	if(tpfirmware){
		release_firmware(tpfirmware);
		tpfirmware = NULL;
	}
err_end:	
	return ret;
}



 int wmt_ts_get_gpionum(void)
{
	return irq_gpio;
}

int wmt_ts_get_resetgpnum(void)
{
	return rst_gpio;
}

int wmt_ts_get_resolvX(void)
{
	return panelres_x;
}

int wmt_ts_get_resolvY(void)
{
	return panelres_y;
}

//up:1-pen up,0-pen down
void wmt_ts_set_penup(int up)
{
	l_penup = up;
}

//
int wmt_ts_wait_penup(void)
{
	int ret = wait_event_interruptible(
			ts_penup_wait_queue,
			(1==l_penup));
	return ret;
}

// return:1-pen up,0-pen dwon
int wmt_ts_ispenup(void)
{
	return l_penup;
}


void wmt_ts_wakeup_penup(void)
{
	wake_up(&ts_penup_wait_queue);
}

int wmt_is_tsirq_enable(void)
{
	int val = 0;
	int num = irq_gpio;
	
	if(num > 11)
		return 0;

	if(num<4)
		val = REG32_VAL(__GPIO_BASE+0x0300) & (1<<(num*8+7)); 
	else if(num >= 4 && num < 8)
		val = REG32_VAL(__GPIO_BASE+0x0304) & (1<<((num-4)*8+7)); 
	else
		val = REG32_VAL(__GPIO_BASE+0x0308) & (1<<((num-8)*8+7));  
	
	return val?1:0;

}

int wmt_is_tsint(void)
{
	int num = irq_gpio;
	
	if (num > 11)
	{
		return 0;
	}
	return (REG32_VAL(__GPIO_BASE+0x0360) & (1<<num)) ? 1: 0; 
}

void wmt_clr_int(void)
{
	int num = irq_gpio;
	
	if (num > 11)
	{
		return;
	}
	REG32_VAL(__GPIO_BASE+0x0360) = 1<<num;
}

void wmt_tsreset_init(void)
{
	int num = rst_gpio;
	
	REG32_VAL(__GPIO_BASE+0x0040) |= (1<<num);//&= ~(1<<num); //enable gpio
	REG32_VAL(__GPIO_BASE+0x00C0) &= ~(1<<num); // out low
	REG32_VAL(__GPIO_BASE+0x0080) |= (1<<num); //output enable
	msleep(10);
	REG32_VAL(__GPIO_BASE+0x00C0) |= (1<<num); // out high
}

// enable:0-disable,1-enable
void wmt_enable_rst_pull(int enable)
{
	if (enable)
	{
		REG32_VAL(__GPIO_BASE+0x0480) |= (1<<rst_gpio); //enable pull up/down
	} else {
		REG32_VAL(__GPIO_BASE+0x0480) &= ~(1<<rst_gpio); //disable pull up/down
	}
}

// up:0-pull down,1-pull up
void wmt_set_rst_pull(int up)
{
	if (up)
	{
		REG32_VAL(__GPIO_BASE+0x04c0) |= (1<<rst_gpio); //pull up
	} else {
		REG32_VAL(__GPIO_BASE+0x04c0) &= ~(1<<rst_gpio); //pull down
	}
}

// high:0-low level,1-high level
void wmt_rst_output(int high)
{
	REG32_VAL(__GPIO_BASE+0x0040) |= (1<<rst_gpio); //enable gpio
	if (high)
	{
		REG32_VAL(__GPIO_BASE+0x00C0) |= (1<<rst_gpio); // high
	} else {
		REG32_VAL(__GPIO_BASE+0x00C0) &= ~(1<<rst_gpio); // low
	}
	REG32_VAL(__GPIO_BASE+0x0080) |= (1<<rst_gpio); //set output
}

void wmt_rst_input(void)
{
	REG32_VAL(__GPIO_BASE+0x0040) |= (1<<rst_gpio); //enable gpio
	REG32_VAL(__GPIO_BASE+0x0080) &= ~(1<<rst_gpio); //set input
}

void wmt_set_intasgp(void)
{
	REG32_VAL(__GPIO_BASE+0x0040) |= (1<<irq_gpio); //enable gpio
}

// val:1--high,0-low
void wmt_intgp_out(int val)
{
	if (val)
	{
		REG32_VAL(__GPIO_BASE+0x00C0) |= (1<<irq_gpio); // high
	} else {
		REG32_VAL(__GPIO_BASE+0x00C0) &= ~(1<<irq_gpio); // low
	}
	REG32_VAL(__GPIO_BASE+0x0080) |= (1<<irq_gpio); //set output
}

void wmt_ts_set_irqinput(void)
{
	int num = irq_gpio;
	
	REG32_VAL(__GPIO_BASE+0x0040) |= (1<<num); //enable gpio
	REG32_VAL(__GPIO_BASE+0x0080) &= ~(1<<num); //set input
}

unsigned int wmt_ts_irqinval(void)
{
	REG32_VAL(__GPIO_BASE+0x0040) &= ~(1<<irq_gpio); //enable gpio
	REG32_VAL(__GPIO_BASE+0x0080) &= ~(1<<irq_gpio); //set input
	return REG32_VAL(__GPIO_BASE+0x0000)&(1<<irq_gpio);
}

int wmt_set_gpirq(int type) 
{
	int shift;
	int offset;
	unsigned long reg;
	int num = irq_gpio;
	
	if(num >11)
		return -1;
	//if (num > 9)
		//GPIO_PIN_SHARING_SEL_4BYTE_VAL &= ~BIT4; // gpio10,11 as gpio
	REG32_VAL(__GPIO_BASE+0x0040) &= ~(1<<num); //enable gpio
	REG32_VAL(__GPIO_BASE+0x0080) &= ~(1<<num); //set input
	REG32_VAL(__GPIO_BASE+0x04c0) |= (1<<num); //pull down
	REG32_VAL(__GPIO_BASE+0x0480) |= (1<<num); //enable pull up/down

	//set gpio irq triger type
	if(num < 4){//[0,3]
		shift = num;
		offset = 0x0300;
	}else if(num >= 4 && num < 8){//[4,7]
		shift = num-4;
		offset = 0x0304;
	}else{// [8,11]
		shift = num-8;
		offset = 0x0308;
	}
	
	reg = REG32_VAL(__GPIO_BASE + offset);

	switch(type){
		case IRQ_TYPE_LEVEL_LOW:
			reg &= ~(1<<(shift*8+2)); 
			reg &= ~(1<<(shift*8+1));
			reg &= ~(1<<(shift*8));
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			reg &= ~(1<<(shift*8+2)); 
			reg &= ~(1<<(shift*8+1));
			reg |= (1<<(shift*8));
			break;
		case IRQ_TYPE_EDGE_FALLING:
			reg &= ~(1<<(shift*8+2)); 
			reg |= (1<<(shift*8+1));
			reg &= ~(1<<(shift*8));
			break;
		case IRQ_TYPE_EDGE_RISING:
			reg &= ~(1<<(shift*8+2)); 
			reg |= (1<<(shift*8+1));
			reg |= (1<<(shift*8));
			break;
		default://both edge
			reg |= (1<<(shift*8+2)); 
			reg &= ~(1<<(shift*8+1));
			reg &= ~(1<<(shift*8));
			break;
			
	}
	//reg |= 1<<(shift*8+7);//enable interrupt
	reg &= ~(1<<(shift*8+7)); //disable int

	REG32_VAL(__GPIO_BASE + offset) = reg; 
	REG32_VAL(__GPIO_BASE+0x0360) = 1<<num; //clear interrupt status
	msleep(5);
	return 0;
}

int wmt_enable_gpirq(void)
{
	int num = irq_gpio;
	
	if(num > 11)
		return -1;

	if(num<4)
		REG32_VAL(__GPIO_BASE+0x0300) |= 1<<(num*8+7); //enable interrupt 
	else if(num >= 4 && num < 8)
		REG32_VAL(__GPIO_BASE+0x0304) |= 1<<((num-4)*8+7); //enable interrupt 
	else
		REG32_VAL(__GPIO_BASE+0x0308) |= 1<<((num-8)*8+7); //enable interrupt 
	
	return 0;
}

int wmt_disable_gpirq(void)
{
	int num = irq_gpio;
	
	if(num > 11)
		return -1;
	
	if(num<4)
		REG32_VAL(__GPIO_BASE+0x0300) &= ~(1<<(num*8+7)); //enable interrupt 
	else if(num >= 4 && num < 8)
		REG32_VAL(__GPIO_BASE+0x0304) &= ~(1<<((num-4)*8+7)); //enable interrupt 
	else
		REG32_VAL(__GPIO_BASE+0x0308) &= ~(1<<((num-8)*8+7)); //enable interrupt 
	
	return 0;
}


int wmt_get_tsirqnum(void)
{
	return IRQ_GPIO;
}


int wmt_ts_set_rawcoord(unsigned short x, unsigned short y)
{
	g_evLast.x = x;
	g_evLast.y = y;
	//dbg("raw(%d,%d)*\n", x, y);
	return 0;
}

static void wmt_ts_platform_release(struct device *device)
{
    return;
}

static struct platform_device wmt_ts_plt_device = {
    .name           = TS_DRIVER_NAME,
    .id             = 0,
    .dev            = {
        .release = wmt_ts_platform_release,
    },
//    .num_resources  = ARRAY_SIZE(wm9715_ts_resources),
//    .resource       = wm9715_ts_resources,
};

static int wmt_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	dbg("ts suspend....\n");
	if (l_tsdev->suspend != NULL)
	{
		return l_tsdev->suspend(pdev, state);
	}
	return 0;
}
static int wmt_ts_resume(struct platform_device *pdev)
{
	dbg("ts resume....\n");
	if (l_tsdev->resume != NULL)
	{
		return l_tsdev->resume(pdev);
	}
	klog("...\n");
	return 0;
}

static int wmt_ts_probe(struct platform_device *pdev)
{
	l_tsproc= create_proc_entry(WMTTS_PROC_NAME, 0666, NULL/*&proc_root*/);
	if (l_tsproc != NULL)
	{
		l_tsproc->read_proc = ts_readproc;
		l_tsproc->write_proc = ts_writeproc;	
	}

	if (l_tsdev->probe != NULL)
		return l_tsdev->probe(pdev);
	else 
		return 0;
}

static int wmt_ts_remove(struct platform_device *pdev)
{
	if (l_tsproc != NULL)
	{
		remove_proc_entry(WMTTS_PROC_NAME, NULL);
		l_tsproc = NULL;
	}

	if (l_tsdev->remove != NULL)
		return l_tsdev->remove(pdev);
	else
		return 0;
}

static struct platform_driver wmt_ts_plt_driver = {
	.driver = {
	             .name = TS_DRIVER_NAME,
		     .owner	= THIS_MODULE,
	 },
	.probe = wmt_ts_probe,
	.remove = wmt_ts_remove,
	.suspend        = wmt_ts_suspend,
	.resume         = wmt_ts_resume,
};

static int wmt_ts_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	klog("wmt ts driver opening...\n");

	//ts_clear();
	//try_module_get(THIS_MODULE);

    	return ret;
}

static int wmt_ts_close(struct inode *inode, struct file *filp)
{
	klog("wmt ts driver closing...\n");
	//ts_clear();
    	//module_put(THIS_MODULE);

	return 0;
}

static unsigned int wmt_ts_poll(struct file *filp, struct poll_table_struct *wait)
{
#if 0
    	poll_wait(filp, &queue, wait);
    	if ( head != tail )
        	return (POLLIN | POLLRDNORM);
#endif
    return 0;
}

static long wmt_ts_ioctl(/*struct inode * node,*/ struct file *dev, unsigned int cmd, unsigned long arg)
{
	int nBuff[7];
	char env_val[96]={0};
	//dbg("wmt_ts_ioctl(node=0x%p, dev=0x%p, cmd=0x%08x, arg=0x%08lx)\n", node, dev, cmd, arg);

	if (_IOC_TYPE(cmd) != TS_IOC_MAGIC){ 
		dbg("CMD ERROR!");
		return -ENOTTY;
	}
	
	if (_IOC_NR(cmd) > TS_IOC_MAXNR){ 
		dbg("NO SUCH IO CMD!\n");
		return -ENOTTY;
	}

	switch (cmd) {
			
		case TS_IOCTL_CAL_DONE:
			klog("wmt_ts_ioctl: TS_IOCTL_CAL_DONE\n");
			copy_from_user(nBuff, (unsigned int*)arg, 7*sizeof(int));

			mutex_lock(&cal_mutex);
			g_CalcParam.a1 = nBuff[0];
			g_CalcParam.b1 = nBuff[1];
			g_CalcParam.c1 = nBuff[2];
			g_CalcParam.a2 = nBuff[3];
			g_CalcParam.b2 = nBuff[4];
			g_CalcParam.c2 = nBuff[5];
			g_CalcParam.delta = nBuff[6];

			if(g_CalcParam.delta == 0)
				g_CalcParam.delta =1;//avoid divide by zero

			mutex_unlock(&cal_mutex);
			
			sprintf(env_val,"%d %d %d %d %d %d %d",nBuff[0],nBuff[1],nBuff[2],nBuff[3],nBuff[4],nBuff[5],nBuff[6]);
			wmt_setsyspara("wmt.io.ts.2dcal", env_val);
			klog("Tsc calibrate done data: [%s]\n",env_val);
			
			return 0;
			
			
		case TS_IOCTL_GET_RAWDATA:
			//  wait for point up
			dbg("test wait_penup\n");
			//l_tsdev->wait_penup(&gt811_tsdev);
			klog("wmt_ts_ioctl: TS_IOCTL_GET_RAWDATA\n");
			wmt_ts_wait_penup();
			
			nBuff[0] = g_evLast.x;
			nBuff[1] = g_evLast.y;
			copy_to_user((unsigned int*)arg, nBuff, 2*sizeof(int));
			klog("raw data: x=%d, y=%d\n", nBuff[0], nBuff[1]);
			
			return 0;
	}
	
	return -EINVAL;
}

static ssize_t wmt_ts_read(struct file *filp, char *buf, size_t count, loff_t *l)
{
	// read firmware file 
	
	return 0;
}


static struct file_operations wmt_ts_fops = {
	.read           = wmt_ts_read,
	.poll           = wmt_ts_poll,
	.unlocked_ioctl = wmt_ts_ioctl,
	.open           = wmt_ts_open,
	.release        = wmt_ts_close,
};

static int ts_writeproc( struct file   *file,
                           const char    *buffer,
                           unsigned long count,
                           void          *data )
{
	int calibrate = 0;
	int val = 0;

	if (sscanf(buffer, "calibrate=%d\n", &calibrate))
	{
		if (1 == calibrate)
		{
			if((l_tsdev->capacitance_calibrate != NULL) && 
				(0 == l_tsdev->capacitance_calibrate()))
			{
				printk(KERN_ALERT "%s calibration successfully!\n", l_tsdev->ts_id);
			} else {
				printk(KERN_ALERT "%s calibration failed!\n", l_tsdev->ts_id);
			}
		}
	} else if (sscanf(buffer, "reset=%d\n", &val))
	{
		
	}
	return count;
}

static int ts_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len = 0;

	len = sprintf(page, 
			"echo calibrate=1 > /proc/wmtts_config   to calibrate ts.\n");
	return len;
}

static int wmt_check_touch_env(void)
{
	int ret = 0;
	int len = 127, i = 0;
    char retval[128] = {0},*p=NULL,*s=NULL;
	int Enable=0;
	int keypos[4][4];
	
    	// Get u-boot parameter
	ret = wmt_getsyspara("wmt.io.touch", retval, &len);
	if(ret){
		errlog("Read wmt.io.touch Failed.\n");
		return -EIO;
	}
	memset(l_tpinfor,0,sizeof(l_tpinfor[0]));
	p = retval;
	sscanf(p,"%d:", &Enable);
	p = strchr(p,':');p++;
	s = strchr(p,':');
	strncpy(l_tpinfor[0].name,p, (s-p));
	p = s+1;
	dbg("ts_name=%s\n", l_tpinfor[0].name);
#ifdef	GSL_NOID_VERSION
	if (!strncmp(l_tpinfor[0].name, "GSL3670", 7)||!strncmp(l_tpinfor[0].name, "GSL3680B", 8))
		gsl_noid_ver = 1;
	dbg("gsl_noid_ver = %d\n", gsl_noid_ver);
#endif
	
	ret = sscanf(p,"%d:%d:%d:%d:%d:%d:%d:%d:%d",
		&irq_gpio,&panelres_x,&panelres_y,&rst_gpio,
		&(l_tpinfor[0].xaxis),&(l_tpinfor[0].xdir),&(l_tpinfor[0].ydir),
		&(l_tpinfor[0].finger_num),
		&(l_tpinfor[0].i2caddr));
	if (ret != 9)
	{
		dbg("Wrong format ts u-boot param(%d)!\n",ret);
		return -ENODEV;
	}
	//check touch enable
	if(Enable == 0){
		errlog("Touch Screen Is Disabled.\n");
		return -ENODEV;
	}

	/*if (strstr(l_tpinfor[0].name, l_tsdev->ts_id) == NULL)
	{
		dbg("Can't find %s!\n", l_tsdev->ts_id);
		return -ENODEV;
	}*/
	l_tpindex = 0;
	
	klog("p.x = %d, p.y = %d, gpio=%d, resetgpio=%d,xaxis=%d,xdir=%d,ydri=%d,i2caddr=%d\n", 
	          panelres_x, panelres_y, irq_gpio, rst_gpio,
	          l_tpinfor[0].xaxis,l_tpinfor[0].xdir,l_tpinfor[0].ydir,
	          l_tpinfor[0].i2caddr);


	memset(retval,0,sizeof(retval));
	ret = wmt_getsyspara("wmt.io.tskey", retval, &len);
	if(ret)
	{
		l_iftskey = 0;
	} else {
		// get touch key
		p = retval;
		sscanf(p,"%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d", \
		       keypos[0],keypos[0]+1,keypos[0]+2,keypos[0]+3, \
		       keypos[1],keypos[1]+1,keypos[1]+2,keypos[1]+3, \
		       keypos[2],keypos[2]+1,keypos[2]+2,keypos[2]+3, \
		       keypos[3],keypos[3]+1,keypos[3]+2,keypos[3]+3);
		for (i = 0; i < 4; i++)
		{
			wmt_set_keypos(i,keypos[i][0],keypos[i][1],keypos[i][2],keypos[i][3]);
			dbg("%d:%d,%d,%d,%d\n",i,keypos[i][0],keypos[i][1],keypos[i][2],keypos[i][3]);
		};
		l_iftskey = 1;
	} 

	memset(retval,0,sizeof(retval));
	ret = wmt_getsyspara("wmt.touch.earlysus", retval, &len);
	if(!ret) {
		p = retval;
		sscanf(p, "%d", &earlysus_en);
	}
		
/*
	memset(retval,0,sizeof(retval));
    	ret = wmt_getsyspara("wmt.io.ts.2dcal", retval, &len);
	if(ret){
		errlog("Read env wmt.io.ts.2dcal Failed.\n ");
		//return -EIO;
	}
	i = 0;
	while(i < sizeof(retval)){
		if(retval[i]==' ' || retval[i]==',' || retval[i]==':')
			retval[i] = '\0';
		i++;
	}
	
	i = 0;
	p = retval;
	while(i<7 && p < (retval + sizeof(retval))){
		if(*p == '\0')
			p++;
		else{
			sscanf(p,"%d",&nBuff[i]);
			//printk("%d\n",nBuff[i]);
			p=p+strlen(p);
			i++;
		}
	}
	//sscanf(retval,"%d %d %d %d %d %d %d %d",&nBuff[0],&nBuff[1],&nBuff[2],&nBuff[3],&nBuff[4],&nBuff[5],&nBuff[6]);
	printk("Tsc calibrate init data: [%d %d %d %d %d %d %d]\n",nBuff[0],nBuff[1],nBuff[2],nBuff[3],nBuff[4],nBuff[5],nBuff[6]);

	g_CalcParam.a1 = nBuff[0];
	g_CalcParam.b1 = nBuff[1];
	g_CalcParam.c1 = nBuff[2];
	g_CalcParam.a2 = nBuff[3];
	g_CalcParam.b2 = nBuff[4];
	g_CalcParam.c2 = nBuff[5];
	g_CalcParam.delta = nBuff[6];

	if(g_CalcParam.delta == 0)
		g_CalcParam.delta =1;//avoid divide by zero
*/
	return 0;
}

struct i2c_board_info ts_i2c_board_info = {
	.type          = WMT_TS_I2C_NAME,
	.flags         = 0x00,
	//.addr          = WMT_TS_I2C_ADDR,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

static int ts_i2c_register_device (void)
{
	struct i2c_board_info *ts_i2c_bi;
	struct i2c_adapter *adapter = NULL;
	//struct i2c_client *client   = NULL;

	ts_i2c_board_info.addr =  l_tpinfor[0].i2caddr;
	ts_i2c_bi = &ts_i2c_board_info;
	adapter = i2c_get_adapter(1);/*in bus 1*/

	if (NULL == adapter) {
		printk("can not get i2c adapter, client address error\n");
		return -1;
	}
	l_client = i2c_new_device(adapter, ts_i2c_bi);
	if (l_client == NULL) {
		printk("allocate i2c client failed\n");
		return -1;
	}
	i2c_put_adapter(adapter);
	return 0;
}

static void ts_i2c_unregister_device(void)
{
	if (l_client != NULL)
	{
		i2c_unregister_device(l_client);
		l_client = NULL;
	}
}

struct i2c_client* ts_get_i2c_client(void)
{
	return l_client;
}

static int __init wmt_ts_init(void)
{
	int ret = 0;
  
	if(wmt_check_touch_env())
		return -ENODEV;

	if (ts_i2c_register_device()<0)
	{
		dbg("Error to run ts_i2c_register_device()!\n");
		return -1;
	}
	mutex_init(&cal_mutex);
	
	if (l_tsdev->init() < 0){
		dbg("Errors to init %s ts IC!!!\n", l_tsdev->ts_id);
		ret = -1;
		goto err_init;
	}
	// Create device node
	if (register_chrdev (TS_MAJOR, TS_NAME, &wmt_ts_fops)) {
		printk (KERN_ERR "wmt touch: unable to get major %d\n", TS_MAJOR);
		return -EIO;
	}	
	
	l_dev_class = class_create(THIS_MODULE, TS_NAME);
	if (IS_ERR(l_dev_class)){
		ret = PTR_ERR(l_dev_class);
		printk(KERN_ERR "Can't class_create touch device !!\n");
		return ret;
	}
	l_clsdevice = device_create(l_dev_class, NULL, MKDEV(TS_MAJOR, 0), NULL, TS_NAME);
	if (IS_ERR(l_clsdevice)){
		ret = PTR_ERR(l_clsdevice);
		printk(KERN_ERR "Failed to create device %s !!!",TS_NAME);
		return ret;
	}
	
	// register device and driver of platform
    	ret = platform_device_register(&wmt_ts_plt_device);
    	if(ret){
			errlog("wmt ts plat device register failed!\n");
		return ret;
    	}
    	ret = platform_driver_register(&wmt_ts_plt_driver);
    	if(ret){
        	errlog("can not register platform_driver_register\n");
        	platform_device_unregister(&wmt_ts_plt_device);
        	return ret;
    	}

	klog("%s driver init ok!\n",l_tsdev->ts_id);
	return 0;
err_init:
	ts_i2c_unregister_device();
	return ret;
}

static void __exit wmt_ts_exit(void)
{
	dbg("%s\n",__FUNCTION__);
	
	l_tsdev->exit();	
	platform_driver_unregister(&wmt_ts_plt_driver);
	platform_device_unregister(&wmt_ts_plt_device);
	device_destroy(l_dev_class, MKDEV(TS_MAJOR, 0));
	unregister_chrdev(TS_MAJOR, TS_NAME);
	class_destroy(l_dev_class);
	mutex_destroy(&cal_mutex);
	ts_i2c_unregister_device();
}


module_init(wmt_ts_init);
module_exit(wmt_ts_exit);

MODULE_LICENSE("GPL");

