/*
 * my_mem_manager.h
 *
 *  Created on: Oct 25, 2018
 *      Author: sg1425
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef MY_MEM_MANAGER_H_
#define MY_MEM_MANAGER_H_

#define THREADREQ 1234

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

#define MAIN_MEM_SIZE 1024*1024//*8
#define SWAP_SIZE 1024*1024*16

//make bitsets of page table entries
typedef struct page_table_entry {
	uint used :1;
	uint in_memory :1;	//if not then check in swap
	uint mem_page_no :15;
	uint swap_page_no :12;
} pte;

typedef struct memory_manager {
	int page_size;
	int total_pages;
	pte *page_table;
} mem_manager;

typedef struct inverted_pagetable_entry {
	uint tid :12;	// Allowing maximum 2014 threads
	uint free :1;
	uint offset :12;// Assuming max PAGE_SIZE of the underlying system to be 4096 Bytes.
} inv_pg_entry;

typedef struct swap_data {
	uint valid;
	uint tid :12;
	inv_pg_entry *pg_entry;
} swap;

#endif /* MY_MEM_MANAGER_H_ */
