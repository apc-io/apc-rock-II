#ifndef _LINUX_IRQ_GPIO_H
#define _LINUX_IRQ_GPIO_H


extern int wmt_enable_gpirq(int num);
extern int wmt_disable_gpirq(int num);
extern int wmt_is_tsirq_enable(int num);
extern int wmt_is_tsint(int num);
extern void wmt_clr_int(int num);
extern int wmt_set_gpirq(int num, int type); 


#endif
