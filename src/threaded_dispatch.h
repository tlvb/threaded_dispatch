#ifndef __THREADED_DISPATCH_H__
#define __THREADED_DISPATCH_H__

#include "mqueue.h"
#include <pthread.h>

#define INIT 0x01
#define ALIVE 0x03
#define DEAD 0x04
#define DEAD_BY_REQUEST 0x0C
#define DEAD_BY_QUEUE_ERROR 0x14
#define DEAD_BY_FAILED_INIT 0x24

typedef struct {
	int state;

	mqueue *qtow;
	mqueue *qfrw;

	void *(*worker)(void*, void*);
	void *userdata;

	pthread_t t;
} td_monitor;

void tdm_init(td_monitor *tdm, void*(*worker)(void*, void*), void *userdata);
void tdm_init_shared(td_monitor *tdm, td_monitor *to_worker, td_monitor *from_worker, void*(*worker)(void*, void*), void *userdata);
int tdm_post_to_worker(td_monitor *tdm, void* msg);
void *tdm_get_from_worker(td_monitor *tdm);
void tdm_worker_exit(td_monitor *tdm);
void tdm_clear_to_worker_queue(td_monitor *tdm, void (*free_func)(void*,void*), void *user_data);
void tdm_clear_from_worker_queue(td_monitor *tdm, void (*free_func)(void*,void*), void *user_data);
void tdm_join(td_monitor *tdm);
void tdm_destroy(td_monitor *tdm);

#endif
