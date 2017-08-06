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
	//read_write = 0;
	//ready = false;
	thread = NULL;
	//count = 0;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/
void BlockingDisk::handle_interrupt(REGS *_r) {

	  //ready =  true;
	  if (thread == NULL) return;
          if (!SYSTEM_SCHEDULER->contains(Thread::CurrentThread()))
          	SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
	  SYSTEM_SCHEDULER->terminate(thread);
 	  if (Machine::interrupts_enabled()) Machine::disable_interrupts();
	  Thread::dispatch_to(thread);
}

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
	issue_operation(READ, _block_no);
	if (!is_ready()) {
		thread = Thread::CurrentThread();
		SYSTEM_SCHEDULER->yield();
	}
	int i;
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = Machine::inportw(0x1F0);
		_buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	}
	thread = NULL;
}

void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
	issue_operation(WRITE, _block_no);
	if (!is_ready()) {
		thread = Thread::CurrentThread();
		SYSTEM_SCHEDULER->yield();
	}

	int i;   
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
		Machine::outportw(0x1F0, tmpw);
	}
	//ready = false;
	thread = NULL;
  /* write data to port */
}
