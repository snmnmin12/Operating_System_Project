/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Queue::Queue() 
  {
      head = 0;
      _size = 0;
  }
void Queue::enqueue(Thread *t) {
    Node *temp = new Node(t);
    if (head == 0) {
        head = temp;
    }else {
	Node *p = head;
	while (p->next) p = p->next;
	   p->next = temp;
	}
    _size++;
   } 
Thread* Queue::dequeue() {
    if (!head) return 0;
    Node* p = head;
    head = p->next;
    Thread *t = p->thread;
    delete p;
    _size--;
    return t;
  }
   
void Queue::remove(Thread * thread){
    Node* p = head;
    Node* pre = 0;
    while(p && p->thread->ThreadId() != thread->ThreadId()) {
	pre = p;
	p = p->next;
    }
    if (!p) return;
    if (pre == 0) {
	head = p->next;
    }else{
	pre->next = p->next;
    }
    delete p;
    _size--;
  }

Scheduler::Scheduler() {
  ready_queue = Queue();
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  if (ready_queue.isEmpty()) return;
  Thread* thread = ready_queue.dequeue();
  Thread::dispatch_to(thread);
}

void Scheduler::resume(Thread * _thread) {
  ready_queue.enqueue(_thread);
}

void Scheduler::add(Thread * _thread) {
  ready_queue.enqueue(_thread);
}

void Scheduler::terminate(Thread * _thread) {
   ready_queue.remove(_thread);
}
