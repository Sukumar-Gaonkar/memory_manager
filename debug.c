/*
 * debug.c
 *
 *  Created on: Oct 27, 2018
 *      Author: sg1425
 */

#include "my_pthread_t.h"
#include "my_mem_manager.h"



/*
int main(int argc, char **argv) {
	int *i = malloc(500 * sizeof(int));
	*i = malloc(100 * sizeof(int));
}
*/

void * dummyFunction(tcb *thread) {
	my_pthread_t curr_threadID = thread->tid;
	printf("Entered Thread %i\n", curr_threadID);
	int *iii = malloc(1000 * sizeof(int));
	int i = 0, j = 0, k = 0, l = 0;
	for (i = 0; i < 100; i++) {
		printf("Thread %d: %i\n", curr_threadID, i);

		for (j = 0; j < 50000; j++)
			k++;
	}
	printf("Exited Thread: %i\n", curr_threadID);
	return &(thread->tid);
}

int main(int argc, char **argv) {
	pthread_t t1, t2, t3, t4;
	pthread_create(&t1, NULL, (void *) dummyFunction, &t1);
	pthread_create(&t2, NULL, (void *) dummyFunction, &t2);

	void ** op_val;
	int i = 0, j = 0, k = 0, l = 0;
	for (i = 0; i < 100; i++) {
		printf("Main: %d\n", i);
		if (i == 21) {
			pthread_join(t1, op_val);
		}

		for (j = 0; j < 50000; j++)
			k++;
	}

	printf("Done\n");

	return 0;
}
