#include "page_table.H"
#include "paging_low.H"

FramePool     * PageTable::kernel_mem_pool;    /* Frame pool for the kernel memory */
FramePool     * PageTable::process_mem_pool;   /* Frame pool for the process memory */
unsigned long   PageTable::shared_size;        /* size of shared address space */
PageTable     * PageTable::current_page_table;


PageTable::PageTable() {
    /* Setting up page directory. Find some free memory to in kernel memory pool
       get_frame() will return the frame_no of a free frame then we times it with
       the page size (i.e. 4K) to find the actual position the pointer should be in memory.
     
     */
       page_directory = (unsigned long *) (kernel_mem_pool->get_frame() * PAGE_SIZE);
    
    /* Then we put our page_table, which maping the first 4MB memory in right after our page
        directory.
     
     */
	unsigned long *page_table 	= (unsigned long *) (kernel_mem_pool->get_frame() * PAGE_SIZE);
	unsigned long address = 0; // holds the physical address of where a page is
	unsigned int i;

	// map the first 4MB of memory 
	for(i=0; i<1024; i++)
	{
		page_table[i] = address | 3; // attribute set to: supervisor level, read/write, present(011 in binary)
		address = address+ 4096; // 4096 = 4kb
	}
    

	/*
     **** Filling in the Page Directory Entries ******
     
     */
    //Fill the first entry of the page directory
	page_directory[0] = (unsigned long)page_table; // attribute set to: supervisor level, read/write, present(011 in binary)
	page_directory[0] =page_directory[0] | 3;

	// Fill the rest of the page directory
	for(i=1; i<1024; i++)
	{
		page_directory[i] = 0 | 2; // attribute set to: supervisor level, read/write, not present(010 in binary)
	}
}

void PageTable::load() {
	current_page_table = this;
}

void PageTable::enable_paging() {
	/* All we need to do is put the address of the page directory into CR3
       and set the paging bit(bit 31) of CR0 to 1
     */
    // write_cr3, read_cr3, write_cr0, and read_cr0 all come from the assembly function
    
    write_cr3((unsigned long)(current_page_table->page_directory)); // put that page directory address into CR3
	write_cr0(read_cr0() | 0x80000000); // set the paging bit in CR0 to 1
}

void PageTable::handle_fault(REGS * _r) {
    
    unsigned long address = read_cr2();//Get the address that cuased the page fault
    
	/*check if present attribute is 0, that means a page is missing */
	if ((_r->err_code & 1) == 0) {
		// PAGE NOT PRESENT
		unsigned long * page_table;
		unsigned long * page_dir = current_page_table->page_directory;
        
        /* The Higest 10 bit of the address determine the page table address in the page directory*/
		unsigned long page_dir_index = address  >> 22;
        /* The middle 10 bit determine which entry should we map in the page table*/
		unsigned long page_table_index = (address & (0x03FF << 12)) >> 12;
		unsigned long * physical_address;

        /* Find the address of page_table in memory, which should map the missing page */
		page_table = (unsigned long *) page_dir[page_dir_index];
        
        /* If currently there is no page table, we need to build one first*/
		if ((page_dir[page_dir_index] & 1) == 0) {
            /* Still find some place in kernel memory to put this page_table*/
			page_table = (unsigned long *) (kernel_mem_pool->get_frame() * PAGE_SIZE);
            
            /*Map 1024 entries in the page table to 1024 frames(totally size: 4MB) in the process memory pool*/
		for(int i=0; i<1024; i++) {
                
                /* Find a free frame in process memory pool and map physical_address to this frame */
			physical_address = (unsigned long *) (process_mem_pool->get_frame() * PAGE_SIZE);
                
                // attribute set to: user level, read/write, present(011 in binary)

			page_table[i] = (unsigned long) (physical_address) | 3;
		}

		    		
			page_dir[page_dir_index] = (unsigned long) (page_table); // attribute set to: supervisor level, read/write, present(011 in binary)
			page_dir[page_dir_index] |= 3;

		}
        /* If there is already existing a page_table, we just map it*/

        /* get the physical address we need maping to*/
			physical_address = (unsigned long *) (process_mem_pool->get_frame() * PAGE_SIZE);
        /* Map current entry in the page table to this physical_address */
            
        // attribute set to: user level, read/write, present(111 in binary)
			page_table[page_table_index] = (unsigned long) (physical_address);
            
                       page_table[page_table_index] |= 7;

		


	}	

}

void PageTable::init_paging(FramePool * _kernel_mem_pool,
                          FramePool * _process_mem_pool,
                          const unsigned long _shared_size) {
     kernel_mem_pool  = _kernel_mem_pool;
     process_mem_pool = _process_mem_pool;
     shared_size      = _shared_size;
}
