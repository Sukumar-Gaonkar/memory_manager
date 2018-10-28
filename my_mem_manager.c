/*
 * my_mem_manager.c
 *
 *  Created on: Oct 25, 2018
 *      Author: sg1425
 */

#include "my_pthread_t.h"
#include "my_mem_manager.h"

static char *memory;
inv_pg_entry *invt_pg_table;
pte *page_table;
swap *swap_table;
int mem_manager_init = 0;
FILE *swap_file;


int PAGE_SIZE = 0;
int TOTAL_PAGES = 0;
int PAGES_IN_SWAP = 0;
int MAX_THREADS = 0;

#undef malloc(x)
#undef free(x)

void init_mem_manager() {
	if (mem_manager_init == 0) {

		PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
		TOTAL_PAGES = MAIN_MEM_SIZE / PAGE_SIZE;
		PAGES_IN_SWAP = (SWAP_SIZE / PAGE_SIZE);
		MAX_THREADS = 256;

		page_table = malloc(sizeof(pte));
		swap_table = malloc(sizeof(swap));

		//	Initialize Inverted Page Table and mem-align the pages
		invt_pg_table = malloc(TOTAL_PAGES * sizeof(inv_pg_entry));
		memory = malloc(sizeof(MAIN_MEM_SIZE));

		memory = memalign(PAGE_SIZE, MAIN_MEM_SIZE);

		int i = 0;
		for (i = 0; i < TOTAL_PAGES; i++) {

			invt_pg_table[i].tid = 0;
			invt_pg_table[i].is_alloc = 0;
			invt_pg_table[i].max_free = PAGE_SIZE - sizeof(pgm);
			pgm *pg_addr = (void *) (memory + (i * PAGE_SIZE));
			pg_addr->free = 1;
			pg_addr->size = PAGE_SIZE - sizeof(pgm);
			pg_addr->is_max_block = 1;
		}

		//init swap space
		swap_file = fopen(SWAP_NAME, "w+");

		ftruncate(fileno(swap_file), 16 * 1024 * 1024);
		close(fileno(swap_file));
		mem_manager_init = 1;
	}
}

void split_pg_block(pgm *itr_addr, int size){

	pgm *new_ptr = (void *)itr_addr + sizeof(pgm) + size;
	new_ptr->free = 1;
	new_ptr->size = (void *)itr_addr->size - size - sizeof(pgm);

	itr_addr->free = 0;
	itr_addr->size = size;
}


//have to store the metadata of the page separately and not in an inverted page table
//dealloc with use that metadata to figure out the bytes allocated and mark the page as free
//if the total page is free and not partial
//Figure out total free or partial free????

void * myallocate(size_t size, char *filename, int line_number, int flag) {

	int alloc_complete = 0;
	if (flag != THREADREQ) {
		// TODO: dont call malloc, allocate space from kernel space of the memory
		return malloc(size);
	} else {

		make_scheduler();

		void *ret_val;
		int itr;
		int free_index = -1;
		for (itr = 0; itr < TOTAL_PAGES && alloc_complete == 0; itr++) {

			// Keep track of the first free page found
			if (free_index == -1 && invt_pg_table[itr].is_alloc == 0) {
				free_index = itr;
			}

			if (invt_pg_table[itr].tid == scheduler.running_thread->tid) {
				// The current thread already owns a page.
				if (size > invt_pg_table[itr].max_free) {
					// The current thread has asked for more memory than the size of a PAGE in total.
					// This is not permitted.
					// TODO: This is only for Stage 1. Check for free page.

					printf("Memory for thread %d is full",
							scheduler.running_thread->tid);
					return NULL;
				} else {
					int curr_offset = 0;
					pgm *itr_addr = memory + (itr * PAGE_SIZE);
					while(curr_offset < PAGE_SIZE && !(itr_addr->free == 1 && (itr_addr->size > (sizeof(pgm) + size)))){
						curr_offset += sizeof(pgm) + itr_addr->size;
						itr_addr = (void *)itr_addr + sizeof(pgm) + itr_addr->size;
					}

					split_pg_block(itr_addr, size);

					if(itr_addr->is_max_block == 1){
						itr_addr->is_max_block = 0;

						int curr_offset = 0;
						pgm *temp_addr = memory + (itr * PAGE_SIZE);
						invt_pg_table[itr].max_free = 0;
						pgm *max_addr = NULL;

						while(curr_offset < PAGE_SIZE){
							if(temp_addr->free == 1 && temp_addr->size > invt_pg_table[itr].max_free){
								invt_pg_table[itr].max_free = temp_addr->size;
								max_addr = temp_addr;
							}

							curr_offset += sizeof(pgm) + temp_addr->size;
							temp_addr  = (void *)temp_addr + sizeof(pgm) + temp_addr->size;
						}

						if(max_addr != NULL){
							max_addr->is_max_block = 1;
						}
					}


					ret_val = itr_addr + sizeof(pgm);
//					invt_pg_table[itr].max_free += size;
					alloc_complete = 1;
				}
			}
		}

		// The current thread doesnt own any page
		if (alloc_complete == 0) {
			if (free_index == -1) {
				printf("Main memory out of memory (8MB)!!!!");
				return NULL;
			} else {
				inv_pg_entry *free_pg_entry = &(invt_pg_table[free_index]); //memory + (free_index * PAGE_SIZE);
				free_pg_entry->tid = scheduler.running_thread->tid;
				free_pg_entry->is_alloc = 1;
				pgm *free_page = memory + (free_index * PAGE_SIZE);
				split_pg_block(free_page, size);
				free_pg_entry->max_free = free_pg_entry->max_free - sizeof(pgm) - size;
				free_page->is_max_block = 0;
				((pgm *)((void *)free_page + sizeof(pgm) + size))->is_max_block = 1;
				ret_val = (void *)free_page + sizeof(pgm);
			}
		}
		return ret_val;
	}
}

// TODO: dealloc_thread_mem()

void mydeallocate(void * ptr, char *filename, int line_number, int flag) {

	int alloc_complete = 0;
	if (flag != THREADREQ) {
		free(ptr);
	} else {
		make_scheduler();

		pgm *pg_meta = ptr - sizeof(pgm);
		pg_meta->free = 1;
		int inv_pg_index = (int)pg_meta / (int)PAGE_SIZE;

		if(invt_pg_table[inv_pg_index].max_free < pg_meta->size){
			invt_pg_table[inv_pg_index].max_free = pg_meta->size;
		}

	}
	return;
}

void addPTEntry() {

}

