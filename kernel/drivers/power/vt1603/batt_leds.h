#ifndef _BATT_LEDS_H_
#define _BATT_LEDS_H_

extern int batt_leds_setup(void);
extern void batt_leds_cleanup(void);
extern void batt_leds_suspend_prepare(void);
extern void batt_leds_resume_complete(void);
extern int batt_leds_update(int status);

#endif 	/* #ifndef _BATT_LEDS_H_ */


