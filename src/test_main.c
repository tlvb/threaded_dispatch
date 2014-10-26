#include "threaded_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void free_func(void *message, void *userdata) {
	char *m = (char*)message;
	printf("free_func releasing message \"%s\"\n", m);
	free(m);
}
void *worker(void *message, void *userdata) {
	char *instr = (char*)message;
	int wnum = *(int*)userdata;
	printf("worker%d got message \"%s\" from main\n", wnum, message);

	int msgid;
	sscanf(instr, "%*s %d", &msgid);

	free(message);

	char *outstr = malloc(sizeof(char)*64); // 64 chars should be enough for anyone
	sprintf(outstr, "hello %d from worker%d", msgid, wnum);
	printf("worker1 sending message \"%s\" to main\n", outstr);
	sleep(1);
	return outstr;
}
int main(void) {
	int wnum1 = 1;
	td_monitor tdm1;
	tdm_init(&tdm1, worker, &wnum1);
	int wnum2 = 2;
	td_monitor tdm2;
	tdm_init_shared(&tdm2, &tdm1, NULL, worker, &wnum2);

	for (int i=0; i<16; ++i) {
		char *message = malloc(sizeof(char)*25);
		sprintf(message, "hello %d from main", i);
		tdm_post_to_worker(&tdm1, message);
		message = NULL;

		char *returned = (char*)tdm_get_from_worker(&tdm1);
		if (returned == NULL) {
			printf("nothing in queue currently\n");
		}
		else {
			printf("main got message \"%s\" from worker\n", returned);
			free(returned);
		}
	}

	tdm_worker_exit(&tdm1);
	tdm_worker_exit(&tdm2);
	tdm_join(&tdm1);
	tdm_join(&tdm2);

	printf("clearing to worker queue\n");
	tdm_clear_to_worker_queue(&tdm1, free_func, NULL);
	printf("clearing from worker queues\n");
	tdm_clear_from_worker_queue(&tdm1, free_func, NULL);
	tdm_clear_from_worker_queue(&tdm2, free_func, NULL);

	tdm_destroy(&tdm1);
	tdm_destroy(&tdm2);
}
