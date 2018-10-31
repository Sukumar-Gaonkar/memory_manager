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
#define SWAP_NAME "swap_space.swp"

void switch_thread(int old_tid, int new_tid);

//make bitsets of page table entries
typedef struct page_table_entry {
	uint used :1;
	uint in_memory :1;		//if not then check in swap
	uint dirty: 1;			// is it necessary to write this
	uint mem_page_no :12;
	uint swap_page_no :12;
	struct page_table_entry *next;		// Though a pointer will take large space, a linked list of pte is much smaller than an array allocation.
										// Since all virtual pages of all threads will never be in use a the same time.
} pte;

/*
 * Thread Page Table
 *
 *  A array of linked lists, where each sub array has info for all the virtual pages the thread owns.
 *  Array was used instead of linked list as data structure to represent each virtual page to,
 *  avoid storing a pointer(32 bits) to the next virtual page entry.
 */
pte **th_pg_tb;

typedef struct inverted_pagetable_entry {
	uint tid :12;		// Allowing maximum 2048 threads
	uint is_alloc :1;
//	uint pg_no: 12;		// Used for page replacement Algorithms.
	uint max_free :12;	// Assuming max PAGE_SIZE of the underlying system to be 4096 Bytes.
} inv_pte;

typedef struct pg_metadata {
	uint free :1;
	uint is_max_block: 1;
	uint size :12;
}pgm;

typedef struct swap_data {
	uint valid;
	uint tid :12;
	inv_pte *pg_entry;
} swap;

#endif /* MY_MEM_MANAGER_H_ */
