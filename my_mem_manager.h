/*
 * my_mem_manager.h
 *
 *  Created on: Oct 25, 2018
 *      Author: sg1425
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef MY_MEM_MANAGER_H_
#define MY_MEM_MANAGER_H_

#define THREADREQ 1234

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

#define MAIN_MEM_SIZE 1024*1024


typedef struct memory_manager{
	int page_size;
	int total_pages;
}mem_manager;

typedef struct inverted_pagetable_entry{
	uint tid: 10;	// Allowing maximum 2014 threads
	uint free: 1;
	uint offset: 12;	// Assuming max PAGE_SIZE of the underlying system to be 4096 Bytes.
} inv_pg_entry;



#endif /* MY_MEM_MANAGER_H_ */
