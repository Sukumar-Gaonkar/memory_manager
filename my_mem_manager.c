/*
 * my_mem_manager.c
 *
 *  Created on: Oct 25, 2018
 *      Author: sg1425
 */

#include "my_mem_manager.h"
#include <unistd.h>

mem_manager mem_mgr = {.mem_mgr_init = 0};

void init_mem_manager(){
	mem_mgr.page_size = sysconf(_SC_PAGE_SIZE);
	mem_mgr.mem_mgr_init = 1;
}

void * myallocate(size_t size, char *filename, int line_number , int flag){

	void *ret_val;
//	ret_val = malloc(size);
	return ret_val;
}

void mydeallocate(void * ptr, char *filename, int line_number, int flag){
//	free(ptr);
	return;
}
