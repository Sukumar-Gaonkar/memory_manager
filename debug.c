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

	int i = 0, tot_mem = 0;
	for (i = 0; i < 10; i++) {

		tot_mem += 500;
		printf("Thread : %d  total: %d\n", curr_threadID, tot_mem);
		printf("%p\n", malloc(500));
	}
	printf("Exited Thread: %i\n", curr_threadID);
	return &(thread->tid);
}

int main(int argc, char **argv) {
	pthread_t t1, t2, t3, t4;
	pthread_create(&t1, NULL, (void *) dummyFunction, &t1);
	pthread_create(&t2, NULL, (void *) dummyFunction, &t2);

	int i = 0, tot_mem = 0;
	for (i = 0; i < 10; i++) {

		if(i == 5){
			pthread_join(t1, NULL);
		}else if(i == 7){
			pthread_join(t2, NULL);
		}

		tot_mem += 500;
		printf("Main: tot: %d\n", tot_mem);
		printf("%p\n", malloc(500));
	}

	printf("Done\n");
	void *value_ptr = NULL;
	pthread_exit(value_ptr);
	return 0;
}
