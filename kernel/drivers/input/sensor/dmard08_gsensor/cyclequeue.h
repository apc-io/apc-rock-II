#ifndef __CYCLEQUEUE_163704111637_H
#define __CYCLEQUEUE_163704111637_H

#define DATA_TYPE  short
#define QUEUE_LEN  16

struct que_data {
	DATA_TYPE data[3];
};

extern int clque_in(struct que_data* data);
extern int clque_out(struct que_data* data);
extern int clque_is_full(void);
extern int clque_is_empty(void);
#endif



