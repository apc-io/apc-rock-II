#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include "irq_gpio.h"

int wmt_enable_gpirq(int num)
{
	if(num > 15)
		return -1;

	if(num < 4)
		REG32_VAL(__GPIO_BASE+0x0300) |= 1<<(num*8+7); //enable interrupt 
	else if(num >= 4 && num < 8)
		REG32_VAL(__GPIO_BASE+0x0304) |= 1<<((num-4)*8+7); //enable interrupt 
	else if(num >= 8 && num < 12)
		REG32_VAL(__GPIO_BASE+0x0308) |= 1<<((num-8)*8+7); //enable interrupt 
	else
		REG32_VAL(__GPIO_BASE+0x030C) |= 1<<((num-12)*8+7); //enable interrupt 
	
	return 0;
}

int wmt_disable_gpirq(int num)
{
	if(num > 15)
		return -1;
	
	if(num<4)
		REG32_VAL(__GPIO_BASE+0x0300) &= ~(1<<(num*8+7)); //enable interrupt 
	else if(num >= 4 && num < 8)
		REG32_VAL(__GPIO_BASE+0x0304) &= ~(1<<((num-4)*8+7)); //enable interrupt 
	else if(num >= 8 && num < 12)
		REG32_VAL(__GPIO_BASE+0x0308) &= ~(1<<((num-8)*8+7)); //enable interrupt 
	else
		REG32_VAL(__GPIO_BASE+0x030C) &= ~(1<<((num-12)*8+7)); //enable interrupt 
	
	return 0;
}

int wmt_is_tsirq_enable(int num)
{
	int val = 0;
	
	if(num > 15)
		return 0;

	if(num<4)
		val = REG32_VAL(__GPIO_BASE+0x0300) & (1<<(num*8+7)); 
	else if(num >= 4 && num < 8)
		val = REG32_VAL(__GPIO_BASE+0x0304) & (1<<((num-4)*8+7)); 
	else if(num >= 8 && num < 12)
		val = REG32_VAL(__GPIO_BASE+0x0308) & (1<<((num-8)*8+7));  
	else
		val = REG32_VAL(__GPIO_BASE+0x030C) & (1<<((num-12)*8+7));  
		
	return val?1:0;

}

int wmt_is_tsint(int num)
{
	if (num > 15)
	{
		return 0;
	}
	return (REG32_VAL(__GPIO_BASE+0x0360) & (1<<num)) ? 1: 0; 
}

void wmt_clr_int(int num)
{
	if (num > 15)
	{
		return;
	}
	REG32_VAL(__GPIO_BASE+0x0360) = 1<<num;
}

int wmt_set_gpirq(int num, int type) 
{
	int shift;
	int offset;
	unsigned long reg;
	
	if(num >15)
		return -1;
	//if (num > 9)
		//GPIO_PIN_SHARING_SEL_4BYTE_VAL &= ~BIT4; // gpio10,11 as gpio
	REG32_VAL(__GPIO_BASE+0x0040) &= ~(1<<num);//|=(1<<num);// //enable gpio
	REG32_VAL(__GPIO_BASE+0x0080) &= ~(1<<num); //set input
	REG32_VAL(__GPIO_BASE+0x04c0) |= (1<<num); //pull down
	REG32_VAL(__GPIO_BASE+0x0480) &= ~(1<<num); //enable pull up/down

	//set gpio irq triger type
	if(num < 4){//[0,3]
		shift = num;
		offset = 0x0300;
	}else if(num >= 4 && num < 8){//[4,7]
		shift = num-4;
		offset = 0x0304;
	}else if(num >= 8 && num < 12){//[8,11]
		shift = num-8;
		offset = 0x0308;
	}else{// [12,15]
		shift = num-12;
		offset = 0x030C;
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


