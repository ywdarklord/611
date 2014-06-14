#include"page_table.H"
#include"console.H"

FramePool* PageTable::process_mem_pool;
FramePool* PageTable::kernel_mem_pool;
PageTable* PageTable::current_page_table;
unsigned long PageTable::shared_size;


PageTable::PageTable(){
 /* Setting up page directory. Find some free memory to in process memory pool
       get_frame() will return the frame_no of a free frame then we times it with
       the page size (i.e. 4K) to find the actual position the pointer should be in memory.
     
     */
	page_directory = (unsigned long*)(process_mem_pool->get_frame()*PAGE_SIZE);
	unsigned long *page_table = (unsigned long*)(process_mem_pool->get_frame()*PAGE_SIZE);
	unsigned long fault_addr = 0;
	unsigned int i = 0;
	registered_pool_no = 0;
 /* Then we put our page_table, which maping the first 4MB memory in right after our page
        directory.
     
     */
	for(i = 0; i<ENTRIES_PER_PAGE;++i){
		page_table[i] = fault_addr|3;
		fault_addr += 4096;
	}
	/*
     **** Filling in the Page Directory Entries ******
     
     */
	page_directory[0] = (unsigned long)(page_table);
	page_directory[0] |= 3;
	
	for(i=1; i<ENTRIES_PER_PAGE-1; ++i){
		page_directory[i] = 0|2;
	}
       /* Using the recursive trick, let last PDE point to page directory itself */
	page_directory[1023] = (unsigned long)(page_directory)|3;
}

void PageTable::init_paging(FramePool * _kernel_mem_pool,
							FramePool * _process_mem_pool,
							const unsigned long _shared_size){ 
	kernel_mem_pool = _kernel_mem_pool;
	process_mem_pool = _process_mem_pool;
	shared_size = _shared_size;
}

void PageTable::load(){
	current_page_table = this;
}

void PageTable::enable_paging(){
	write_cr3((unsigned long)(current_page_table->page_directory));
	write_cr0(read_cr0() | 0x80000000);
}

void PageTable::handle_fault(REGS *_r){
	FramePool* current_mem_pool;
	int err_code = _r->err_code & 7;
	unsigned long fault_addr = read_cr2();
	
	int c = 0;
        /*Find the corresponding virtual memory pool the fault fault_addr is within*/
	for(; c<current_page_table->registered_pool_no; c++){
		if(current_page_table->registered_VMPools[c]->is_legitimate(fault_addr)){
			current_mem_pool = current_page_table->registered_VMPools[c]->get_frame_pool();
			break;
		}
	}
	if(c==current_page_table->registered_pool_no){
		Console::puts("invalid fault_addr, fault_addr is not within any virtual memory pool");
		for(;;);
	}

	if((err_code & 0x01)==0){

		unsigned long page_dir_index = fault_addr  >> 22;
		unsigned long page_table_index = (fault_addr >>12) & 0x3FF;
		//Find the physical address of page directory
		unsigned long *page_dir = (unsigned long*)(0xFFFFF000); 
		unsigned long page_dir_entry;	
		// find the physical address of a page table get the pointer	
		unsigned long *page_table_ptr = (unsigned long*)((0x3FF<<22)+(page_dir_index<<12));  
		unsigned long page_entry;					

                //If the Page directory entry is empty, we first need to put an frame into it
		if((page_dir[page_dir_index] & 1)==0){
                       //Find an free frame in processor memory pool and map it to page directory
			page_dir_entry = (process_mem_pool->get_frame()*PAGE_SIZE)|3;
			
			page_dir[page_dir_index] = page_dir_entry;
			//Get the pointer pointing to the page table 
			page_table_ptr = (unsigned long*)((0x3FF<<22)+(page_dir_index<<12));
			for(int i=0; i<ENTRIES_PER_PAGE; ++i){
				page_entry = (current_mem_pool->get_frame()*PAGE_SIZE)|3;
				page_table_ptr[i] = page_entry;
			}
                //If a page table is already there, we just map the fault_address
		}else if((page_table_ptr[page_table_index] & 1) ==0){
			page_table_ptr = (unsigned long*)((0x3FF<<22)+(page_dir_index<<12));
			page_entry = (unsigned long)(current_mem_pool->get_frame()*PAGE_SIZE)|3;
			page_table_ptr[page_table_index] = page_entry;
		}
	}
	else{
		Console::puts("Page already there, Cannot happen\n");
		for(;;);
	}
}


void PageTable::free_page(unsigned long _page_no){
	unsigned long frame_no;
	unsigned long page_dir_index = _page_no/ENTRIES_PER_PAGE;
	unsigned long page_table_index = _page_no%ENTRIES_PER_PAGE;
	unsigned long *page_table_ptr = (unsigned long*)((0x3FF<<22)+(page_dir_index<<12));
        //Find the frame number, which is mapped to this page number
	frame_no = (page_table_ptr[page_table_index] &0xFFFFF000)/PAGE_SIZE;
        //Change the last bit to not present i.e 0
	page_table_ptr[page_table_index] &=(0xFFFFFFFE);
	FramePool::release_frame(frame_no);
}

void PageTable::register_vmpool(VMPool *_pool){
	registered_VMPools[registered_pool_no] = _pool;
	registered_pool_no ++;
}


FramePool* PageTable::get_kernal_mem_pool(){
	return kernel_mem_pool;
}
