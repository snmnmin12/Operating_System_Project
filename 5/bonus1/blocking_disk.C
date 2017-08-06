/*
     File        : blocking_disk.c

     Author      : Mingmin Song
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "thread.H"
#include "scheduler.H"
extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
	locked_write = false;
	locked_read = false;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/
/*
void BlockingDisk::wait_until_ready() {
        if (!is_ready()) { 
		Console::puts("Read/Write is not ready, I just give my CPU time to next!\n");
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        	SYSTEM_SCHEDULER->yield();
	}
     }
*/

void BlockingDisk::lock_read(){locked_read = true;}
void BlockingDisk::unlock_read(){locked_read = false;} 

void BlockingDisk::lock_write(){locked_write = true;}
void BlockingDisk::unlock_write(){locked_write = false;} 



void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  //SimpleDisk::read(_block_no, _buf);
  issue_operation(READ, _block_no);

  if (!is_ready()) {
        Console::puts("CPU is switched to the other devices!\n");
   	SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        SYSTEM_SCHEDULER->yield();
   }
	
   //add lock to read
   /*
   if (locked_read) {
	SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        SYSTEM_SCHEDULER->yield();
   }	
	
  //disable the interrupt if interrupt enabled,
  //lock the below operation
  bool interupt_state_changed = false;
  if (Machine::interrupts_enabled()) {
	Machine::disable_interrupts();
	interupt_state_changed = true;
  }
  lock_read(); 
  */
  /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
 	tmpw = Machine::inportw(0x1F0);
	_buf[i*2]   = (unsigned char)tmpw;
	_buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }

  //unlock it.
  /* unlock_read();
  if (interupt_state_changed) {
	Machine::enable_interrupts();
	interupt_state_changed = false;
  }*/

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  //SimpleDisk::read(_block_no, _buf);
   issue_operation(WRITE, _block_no);

   if (!is_ready()) {
   	SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        SYSTEM_SCHEDULER->yield();
   }
   //add lock to write
   /*
   if (locked_write) {
	SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        SYSTEM_SCHEDULER->yield();
   }	
	
  //disable the interrupt if interrupt enabled,
  //lock the below operation
  bool interupt_state_changed = false;
  if (Machine::interrupts_enabled()) {
	Machine::disable_interrupts();
	interupt_state_changed = true;
  }
  lock_write(); */
  /* write data to port */
  int i; 
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }
  /*unlock it.
  unlock_write();
  if (interupt_state_changed) {
	Machine::enable_interrupts();
	interupt_state_changed = false;;
  }*/

}
