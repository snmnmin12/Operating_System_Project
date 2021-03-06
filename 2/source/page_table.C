/*
 File: page_table.C
 Author: Mingmin Song
 Date  : 6/21/2017
 */

#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
   if (page_directory == NULL) {
  	page_directory = (unsigned long *)(kernel_mem_pool->get_frames(1)*PAGE_SIZE);
   	unsigned long *page_table  = (unsigned long *)(kernel_mem_pool->get_frames(1)*PAGE_SIZE);
	//map the first 4MB of memory
	unsigned long address=0; // holds the physical address of where a page is
	//map the first 4MB of memory
	for (int i = 0; i < 1024; i++)
	{
	    page_table[i] = address | 3; // attribute set to: supervisor level, read/write, present(011 in binary)
	    address = address + PAGE_SIZE; // 4096 = 4kb
	}
	page_directory[0] =(unsigned long) page_table|3;
   }
   for (int i = 1; i < 1024; i++)
	page_directory[i] = 0|2;

   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table =  this;
   write_cr3((unsigned long)page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   paging_enabled = 1;
   write_cr0(read_cr0() | 0x80000000);
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  unsigned long *cur_page_directory = (unsigned long*) read_cr3();;
  unsigned long *page_table;
  unsigned int int_no = _r->int_no;
  if (int_no == 0) {
	Console::puts("Division by Zero is Detected!\n");
   } else if (int_no == 14) {
       // |31-22|21-12|11-0 address composition
       // I need to get the page table address from page directory
       // I need to get the page frame address from page table
       unsigned long page_addr = read_cr2();
       unsigned long page_table_index = page_addr>>22;
       unsigned long page_frame_index = ((page_addr>>12) & 0x3FF);
       if ((cur_page_directory[page_table_index] & 1) != 1) {
           //page table not exists, then I need to create one page table for this
	   cur_page_directory[page_table_index] = (unsigned long)((kernel_mem_pool->get_frames(1)*PAGE_SIZE) | 3);
  	   page_table = (unsigned long*)(cur_page_directory[page_table_index] & 0xFFFFF000); //make sure the address start at 4kb 
	   //initialize the empty page table
	   for (int i = 0; i < 1024; i++) 
	        page_table[i] = 0|2;
       }
       // the page table address start from 21th bit to 12th bits in the address
       /// we need to take it out
       page_table = (unsigned long*)(cur_page_directory[page_table_index] & 0xFFFFF000); // make sure the address start at 4 KB
       page_table[page_frame_index] = (unsigned long)(process_mem_pool->get_frames(1)*PAGE_SIZE) | 3;
   }
  Console::puts("handled page fault\n");
}

