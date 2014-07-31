#include <linux/mutex.h>


#include "cyclequeue.h"

static struct que_data que[QUEUE_LEN];
static unsigned int head = -1; // point to the first valaid queue data
static unsigned int tail = 0;  // point to the next to the last valaid queue data
static DEFINE_MUTEX(que_mutex);

// Whether queue is full
// return 1--full,0 -- no full
int clque_is_full(void)
{
	int ret = 0;

	mutex_lock(&que_mutex);
	ret = ((tail+1)%QUEUE_LEN) == head ? 1 : 0;
	mutex_unlock(&que_mutex);
	return ret;	
}

// Whether queue is empty
// return 1--empty,0--no empty
int clque_is_empty(void)
{
	int ret = 0;	

	mutex_lock(&que_mutex);
	ret = (tail == head) ? 1: 0;
	mutex_unlock(&que_mutex);
	return ret;
}

// add to queue
// return:0--successful,-1--queue is full
int clque_in(struct que_data* data)
{
	/*if (clque_is_full())
	{
		return -1;
	}*/
	mutex_lock(&que_mutex);
	que[tail].data[0] = data->data[0];
	que[tail].data[1] = data->data[1];
	que[tail].data[2] = data->data[2];
	tail = (tail+1)%QUEUE_LEN;
	mutex_unlock(&que_mutex);
	return 0;
}

// out to queue
// return:0--successful,-1--queue is empty
int clque_out(struct que_data* data)
{
	/*if (clque_is_empty())
	{
		return -1;
	}*/
	mutex_lock(&que_mutex);
	data->data[0]= que[head].data[0];
	data->data[1]= que[head].data[1];
	data->data[2]= que[head].data[2];
	head = (head+1)%QUEUE_LEN;
	mutex_unlock(&que_mutex);
	return 0;
}

