/*
 * my_mem_manager.c
 *
 *  Created on: Oct 25, 2018
 *      Author: sg1425
 */

#include "my_pthread_t.h"
#include "my_mem_manager.h"
#include <unistd.h>

static char main_mem[MAIN_MEM_SIZE];
inv_pg_entry *invt_pg_table;
int mem_manager_init = 0;

mem_manager mem_mgr;

#undef malloc(x)
#undef free(x)

void init_mem_manager(){
	if(mem_manager_init == 0){
		mem_mgr.page_size = sysconf(_SC_PAGE_SIZE);
		mem_mgr.total_pages = MAIN_MEM_SIZE/mem_mgr.page_size;
		int i = 0;

	//	Initialize Inverted Page Table
		invt_pg_table = malloc(mem_mgr.total_pages * sizeof(inv_pg_entry));
		for(i=0;i<mem_mgr.total_pages;i++){
			invt_pg_table[i].tid = 0;
			invt_pg_table[i].free = 1;
			invt_pg_table[i].offset = 0;
		}
	}
}

void * myallocate(size_t size, char *filename, int line_number , int flag){

	int alloc_complete = 0;
	if(flag != THREADREQ){
		// TODO: dont call malloc, allocate space from kernel space of the main_mem
		return malloc(size);
	}else{

		make_scheduler();

		void *ret_val;
		int itr;
		int free_index = NULL;
		for(itr = 0; itr < mem_mgr.total_pages; itr++){

			// Keep track of the first free page found
			if(free_index == NULL && invt_pg_table[itr].free == 1)
				free_index = itr;

			if(invt_pg_table[itr].tid == scheduler.running_thread->tid){
				// The current thread already owns a page.
				if(size > mem_mgr.page_size - invt_pg_table[itr].offset){
					// The current thread has asked for more memory than the size of a PAGE in total.
					// This is not permitted.
					// TODO: This is only for Stage 1. Check for free page.

					printf("Memory for thread %d is full", scheduler.running_thread->tid);
					return NULL;
				}else{
					ret_val = main_mem + (itr * mem_mgr.page_size) + invt_pg_table[itr].offset; //&(invt_pg_table[itr]) + invt_pg_table[itr].offset;
					invt_pg_table[itr].offset += size;
					alloc_complete = 1;
				}
			}
		}

		// The current thread doesnt own any page
		if(alloc_complete == 0){
			if(free_index == NULL){
				printf("Main memory out of memory (8MB)!!!!");
				return NULL;
			}else {
				inv_pg_entry *free_page = main_mem + (free_index * mem_mgr.page_size);

				free_page->tid = scheduler.running_thread->tid;
				free_page->free = 0;
				free_page->offset = size;
				ret_val =  free_page;
			}
		}
		return ret_val;
	}
}

void mydeallocate(void * ptr, char *filename, int line_number, int flag){
//	free(ptr);
	make_scheduler();
	return;
}

