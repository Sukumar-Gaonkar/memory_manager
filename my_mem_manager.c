/*
 * my_mem_manager.c
 *
 *  Created on: Oct 25, 2018
 *      Author: sg1425
 */
#include <sys/mman.h>
#include "my_pthread_t.h"
#include "my_mem_manager.h"

static char *memory;
inv_pte *invt_pg_table;
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


struct sigaction mem_access_sigact;

static void mem_access_handler(int sig, siginfo_t *si, void *unused)
{
//   printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
   int page_accessed = (int)(si->si_addr - (void *)memory) / PAGE_SIZE;
   void *accessed_page_addr =  memory + (page_accessed * PAGE_SIZE);

   if(scheduler.running_thread->tid == invt_pg_table[page_accessed].tid){
	   // IF running thread owns the page accessed, then unprotect the page.
	   mprotect(accessed_page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE);
   }else{

	   // Is the required present in Main Memory
	   int i, actual_page;
	   pte *curr_pte = th_pg_tb[scheduler.running_thread->tid];
	   for(i=0;i < page_accessed ; i++){
		   curr_pte = curr_pte->next;
	   }

	   if(curr_pte->in_memory == 1){
		   actual_page = curr_pte->mem_page_no;
		   void *actual_page_addr = memory + (actual_page * PAGE_SIZE);

		   //Unprotect the pages to be swapped
		   mprotect(accessed_page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE);
		   mprotect(actual_page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE);

		   swap_pages(actual_page, page_accessed);

		   // Re-Protect the swapped out page
		   mprotect(actual_page_addr, PAGE_SIZE, PROT_NONE);

	   }else{
		   // Page in Swap file
	   }


   }
}

void switch_thread(int old_tid, int new_tid){
	// Protect all pages
	mprotect(memory, PAGE_SIZE * TOTAL_PAGES,PROT_NONE);

}

void init_mem_manager() {
	if (mem_manager_init == 0) {

		PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
		TOTAL_PAGES = MAIN_MEM_SIZE / PAGE_SIZE;
		PAGES_IN_SWAP = (SWAP_SIZE / PAGE_SIZE);
		MAX_THREADS = 256;

		page_table = (pte*)malloc(sizeof(pte));
		swap_table = (swap*)malloc(sizeof(swap));

		//	Initialize Inverted Page Table and mem-align the pages
		invt_pg_table = malloc(TOTAL_PAGES * sizeof(inv_pte));
		memory = (char*)malloc(sizeof(MAIN_MEM_SIZE));

		memory = (char*)memalign(PAGE_SIZE, MAIN_MEM_SIZE);

		int i = 0, j = 0;
		// Init Inverted Page Table
		for (i = 0; i < TOTAL_PAGES; i++) {

			invt_pg_table[i].tid = 0;
			invt_pg_table[i].is_alloc = 0;
			invt_pg_table[i].max_free = PAGE_SIZE - sizeof(pgm);
			pgm *pg_addr = (void *) (memory + (i * PAGE_SIZE));
			pg_addr->free = 1;
			pg_addr->size = PAGE_SIZE - sizeof(pgm);
			pg_addr->is_max_block = 1;
		}

		//Init Thread Page Table
		th_pg_tb = (pte **) malloc(MAX_THREADS * sizeof(pte *));
		for(i = 0;i < MAX_THREADS; i++){
			th_pg_tb[i] = NULL;
		}

		//init swap space
		swap_file = fopen(SWAP_NAME, "w+");

		ftruncate(fileno(swap_file), 16 * 1024 * 1024);
		close(fileno(swap_file));
		mem_manager_init = 1;

		// Protect all pages
		mprotect(memory, PAGE_SIZE * TOTAL_PAGES,PROT_NONE);

		// Register Handler
		mem_access_sigact.sa_flags = SA_SIGINFO;
		sigemptyset(&mem_access_sigact.sa_mask);
		mem_access_sigact.sa_sigaction = mem_access_handler;

		if (sigaction(SIGSEGV, &mem_access_sigact, NULL) == -1)
		{
			printf("Fatal error setting up signal handler\n");
			exit(EXIT_FAILURE);    //explode!
		}
	}
}

void split_pg_block(pgm *itr_addr, int size){

	pgm *new_ptr = (void *)itr_addr + sizeof(pgm) + size;
	new_ptr->free = 1;
	new_ptr->size = (void *)itr_addr->size - size - sizeof(pgm);

	itr_addr->free = 0;
	itr_addr->size = size;
}

/*
 *	Find a free page in main memory
 */
int find_free_page(){
	int pg_no;

	for(pg_no = 0; pg_no < TOTAL_PAGES && invt_pg_table[pg_no].is_alloc == 1 ; pg_no++);

	if(pg_no == TOTAL_PAGES){
		// No space left in main memory.
		return -1;
	}else{
		return pg_no;
	}
}

void *allocate_in_page(int tid, int pg_no, int size){

	// Update Inverted Page Table
	inv_pte *free_pg_entry = &(invt_pg_table[pg_no]); //memory + (free_index * PAGE_SIZE);
	free_pg_entry->tid = tid;
	free_pg_entry->is_alloc = 1;
	free_pg_entry->max_free = free_pg_entry->max_free - sizeof(pgm) - size;

	pgm *free_page = memory + (pg_no * PAGE_SIZE);
	split_pg_block(free_page, size);
	free_page->is_max_block = 0;
	((pgm *)((void *)free_page + sizeof(pgm) + size))->is_max_block = 1;

	return (void *)free_page + sizeof(pgm);
}

void *init_pte(int pg_no){

	// Create a new Page Table Entry for the page
	pte *new_pte = (pte *)malloc(sizeof(pte));

	new_pte->used = 1;
	new_pte->in_memory = 1;
	new_pte->dirty = 0;
	new_pte->mem_page_no = pg_no;
	new_pte->swap_page_no = 0;
	new_pte->next = NULL;
}

void swap_pages(int pg1, int pg2){

	// Verify if valid swap
	if(pg1 == pg2)
		return;


	// Update Thread Page Table Entries
	int i;
	pte *curr_pte;
	// Update mem_page_no of pg1 to pg2 inside Thread Page Table
	if(invt_pg_table[pg1].is_alloc == 1){
		int pg1_tid = invt_pg_table[pg1].tid;

		curr_pte = th_pg_tb[pg1_tid];
		while(curr_pte != NULL && curr_pte->mem_page_no != pg1){
			curr_pte = curr_pte->next;
		}

		if(curr_pte == NULL){
			printf("Page not found in th_pg_tb!!!\n");
			return;
		}

		curr_pte->mem_page_no = pg2;
	}
	// Update mem_page_no of pg2 to pg1 inside Thread Page Table
	if(invt_pg_table[pg2].is_alloc == 1){
		int pg2_tid = invt_pg_table[pg2].tid;
		curr_pte = th_pg_tb[pg2_tid];
		while(curr_pte != NULL && curr_pte->mem_page_no != pg2){
			curr_pte = curr_pte->next;
		}

		if(curr_pte == NULL){
			printf("Page not found in th_pg_tb!!!\n");
			return;
		}

		curr_pte->mem_page_no = pg1;
	}

	// Update Inverted Page Table
	inv_pte *p1 = &(invt_pg_table[pg1]);
	inv_pte *p2 = &(invt_pg_table[pg2]);
	void *holder = malloc (sizeof(inv_pte));
	memcpy(holder , p1, sizeof(inv_pte));
	memcpy(p1, p2, sizeof(inv_pte));
	memcpy(p2, holder, sizeof(inv_pte));

	void *mem_pg1 = memory + (pg1 * PAGE_SIZE);
	void *mem_pg2 = memory + (pg2 * PAGE_SIZE);

	// Perform actual swap in Main memory
	void *temp = malloc(PAGE_SIZE);
	memcpy(temp, mem_pg1, PAGE_SIZE);
	memcpy(mem_pg1, mem_pg2, PAGE_SIZE);
	memcpy(mem_pg2, temp, PAGE_SIZE);
}

void * myallocate(size_t size, char *filename, int line_number, int flag) {

	int alloc_complete = 0, i = 0, j = 0, old_pg = 0;
	if (flag != THREADREQ) {
		// TODO: dont call malloc, allocate space from kernel space of the memory
		return malloc(size);
	} else {

		make_scheduler();

		void *ret_val;
		int tid = scheduler.running_thread->tid;

		if(th_pg_tb[tid] == NULL){
			// The threads is asking for the page for the first time.

			int vir_pg = 0;						// Since this is thread's first page.
			old_pg = find_free_page(tid, size);

			if(old_pg == -1){
				// No Space left in Main Memory
				printf("Main memory is full!!!\n");
				return NULL;
			}

			void *page = memory + (old_pg * PAGE_SIZE);
			mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE);
			page = memory + (vir_pg * PAGE_SIZE);
			mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE);

			swap_pages(old_pg, vir_pg);

			page = memory + (old_pg * PAGE_SIZE);
			mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE);

			// Create a new Page Table Entry for the page

			pte *new_pte = init_pte(vir_pg);
			th_pg_tb[tid] = new_pte;

			// Unprotect newly assignned page

			ret_val = allocate_in_page(tid, vir_pg, size);

		}else{
//			TODO: find if any page that the thread has, contains enough space for current 'size' allocation.
			int vir_pg = 0;
			pte *curr_pte = th_pg_tb[tid];

			for(vir_pg = 0; curr_pte->next != NULL; vir_pg++){
				if(invt_pg_table[curr_pte->mem_page_no].max_free >= size){
					break;
				}
//				vir_pg++;
				curr_pte = curr_pte->next;
			}

			if(invt_pg_table[curr_pte->mem_page_no].max_free >= size){
				// An existing page with free space more than 'size' is found.

				int pg_no = curr_pte->mem_page_no;

				// Find a block in page more than or equal to 'size'
				int curr_offset = 0;
				pgm *itr_addr = memory + (pg_no * PAGE_SIZE);
				while(curr_offset < PAGE_SIZE && !(itr_addr->free == 1 && (itr_addr->size > (sizeof(pgm) + size)))){
					curr_offset += sizeof(pgm) + itr_addr->size;
					itr_addr = (void *)itr_addr + sizeof(pgm) + itr_addr->size;
				}

				split_pg_block(itr_addr, size);

				// Update max_block in the page
				if(itr_addr->is_max_block == 1){
					itr_addr->is_max_block = 0;

					int curr_offset = 0;
					pgm *temp_addr = memory + (pg_no * PAGE_SIZE);
					invt_pg_table[pg_no].max_free = 0;
					pgm *max_addr = NULL;

					while(curr_offset < PAGE_SIZE){
						if(temp_addr->free == 1 && temp_addr->size > invt_pg_table[pg_no].max_free){
							invt_pg_table[pg_no].max_free = temp_addr->size;
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
//				alloc_complete = 1;

			}else if(vir_pg == TOTAL_PAGES - 1){
				// The thread has used up all its virtual memory.
				printf("The thread %d has used up all his virtual memory!!!", tid);
				return NULL;
			}else {
				// The thread does not currently have a page large enough to hold the 'size' allocation.
				// Allocating a new page.

				old_pg = find_free_page(tid, size);
				if(old_pg == NULL){
					// No Space left in Main Memory
					printf("Main memory is full!!!\n");
					return NULL;
				}

				// Id for new virtual page
				vir_pg++;

				void *page = memory + (old_pg * PAGE_SIZE);
				mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE);
				page = memory + (vir_pg * PAGE_SIZE);
				mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE);

				swap_pages(old_pg, vir_pg);

				page = memory + (old_pg * PAGE_SIZE);
				mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE);

				// Create a new Page Table Entry for the page

				pte *new_pte = init_pte(vir_pg);
				curr_pte->next = new_pte;

				void *new_page = memory + (vir_pg * PAGE_SIZE);
				mprotect(new_page, PAGE_SIZE, PROT_READ | PROT_WRITE);
				ret_val = allocate_in_page(tid, vir_pg, size);
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
		int inv_pg_index = (int)((void *)pg_meta - (void *)memory) / (int)PAGE_SIZE;

		if(invt_pg_table[inv_pg_index].max_free < pg_meta->size){
			invt_pg_table[inv_pg_index].max_free = pg_meta->size;
		}

//		TODO: If neighboring blocks are also free, then merge those blocks.
	}
	return;
}

void addPTEntry() {

}

