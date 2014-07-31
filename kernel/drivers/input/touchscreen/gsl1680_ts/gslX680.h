#ifndef _GSLX680_H_
#define _GSLX680_H_



#define GSL_NOID_VERSION
#ifdef	GSL_NOID_VERSION
struct gsl_touch_info
{
	int x[10];
	int y[10];
	int id[10];
	int finger_num;	
};
extern unsigned int gsl_mask_tiaoping(void);
extern unsigned int gsl_version_id(void);
extern void gsl_alg_id_main(struct gsl_touch_info *cinfo);
extern void gsl_DataInit(int *ret);

extern int gsl_noid_ver;
extern unsigned int gsl_config_data_id[512];
#endif

struct fw_data
{
    //u32 offset : 8;
    //u32 : 0;
    u32 offset;
    u32 val;
};

extern void wmt_set_keypos(int index,int xmin,int xmax,int ymin,int ymax);


#endif
