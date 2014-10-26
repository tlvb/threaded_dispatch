#include "threaded_dispatch.h"
#include <stdlib.h>
#ifdef DISPATCH_VERBOSE
#include <stdio.h>
#define td_dp(...) printf(__VA_ARGS__)
#else
#define td_dp(...)
#endif

void *workershim(void *v) { /*{{{*/
	td_monitor *tdm = (td_monitor*)v;
	tdm->state = ALIVE;
	td_dp("workershim for thread %llx entering loop\n", pthread_self());
	for (;;) {
		int type = -1;
		void *data = NULL;

		/* WAIT FOR INCOMING MESSAGE */
		int r = mq_get_wait(tdm->qtow, &type, &data);

		if (r == 1) {
			if (type == 0) {
				/* CALL WORKER FUNCTION WITH MESSAGE DATA */
				data = tdm->worker(data, tdm->userdata);
				if (data != NULL) {
					/* SEND OUTGOING MESSAGE */
					mq_post(tdm->qfrw, 0, data);
					//pthread_cond_signal(tdm->cfrw);
				}
			}
			else {
				td_dp("workershim for thread %llx death by request\n", pthread_self());
				tdm->state = DEAD_BY_REQUEST;
				return NULL;
			}
		}
		else {
			td_dp("workershim for thread %llx death by queue error\n", pthread_self());
			tdm->state = DEAD_BY_QUEUE_ERROR;
			return NULL;
		}
	}
} /*}}}*/
void tdm_init(td_monitor *tdm, void *(*worker)(void*, void*), void *userdata) { /*{{{*/
	tdm_init_shared(tdm, NULL, NULL, worker, userdata);
} /*}}}*/
void tdm_init_shared(td_monitor *tdm, td_monitor *to_worker, td_monitor *from_worker, void*(*worker)(void*, void*), void *userdata) { /*{{{*/
	/* allocation */
	if (to_worker == NULL) { /*{{{*/
		tdm->qtow = malloc(sizeof(mqueue));
		if (tdm->qtow == NULL) {
			tdm->state = DEAD_BY_FAILED_INIT;
			return;
		}
	}
	else {
		tdm->qtow = to_worker->qtow;
	} /*}}}*/
	if (from_worker == NULL) { /*{{{*/
		tdm->qfrw = malloc(sizeof(mqueue));
		if (tdm->qfrw == NULL) {
			if (to_worker == NULL) {
				free(tdm->qtow);
			}
			tdm->state = DEAD_BY_FAILED_INIT;
			return;
		}
	}
	else {
		tdm->qfrw = from_worker->qfrw;
	} /*}}}*/
	/* initialization */
	if (to_worker == NULL) {
		mq_init(tdm->qtow);
	}
	if (from_worker == NULL) {
		mq_init(tdm->qfrw);
	}
	tdm->worker = worker;
	tdm->userdata = userdata;
	tdm->state = INIT;
	pthread_create(&tdm->t, NULL, workershim, tdm);
} /*}}}*/
int tdm_post_to_worker(td_monitor *tdm, void* msg) { /*{{{*/
	mq_post(tdm->qtow, 0, msg);
} /*}}}*/
void *tdm_get_from_worker(td_monitor *tdm) { /*{{{*/
	int type = 0;
	void *data = NULL;
	if (mq_get(tdm->qfrw, &type, &data) == 1) {
		if (type != 0) {
			free(data);
			data = NULL;
		}
	}
	return data;
} /*}}}*/
void tdm_worker_exit(td_monitor *tdm) { /*{{{*/
	mq_post(tdm->qtow, 1, NULL);
} /*}}}*/
void free_func_shim(int type, void *msg, void *shimdata) { /*{{{*/
	ffs_data *ffsd = (ffs_data*)shimdata;
	if (type == 0) {
		ffsd->ff(msg, ffsd->ud);
	}
	else {
		free(msg);
	}
} /*}}}*/
void tdm_clear_to_worker_queue(td_monitor *tdm, void (*free_func)(void*,void*), void *user_data) { /*{{{*/
	ffs_data ffsd = { .ff = free_func, .ud = user_data };
	mq_purge(tdm->qtow, free_func_shim, &ffsd);
} /*}}}*/
void tdm_clear_from_worker_queue(td_monitor *tdm, void (*free_func)(void*,void*), void *user_data) { /*{{{*/
	ffs_data ffsd = { .ff = free_func, .ud = user_data };
	mq_purge(tdm->qfrw, free_func_shim, &ffsd);
} /*}}}*/
void tdm_join(td_monitor *tdm) { /*{{{*/
	pthread_join(tdm->t, NULL);
} /*}}}*/
void tdm_destroy(td_monitor *tdm) { /*{{{*/
	mq_destroy(tdm->qtow);
	mq_destroy(tdm->qfrw);
} /*}}}*/
