#include "thread.h"
#include <sys/queue.h>
#include <ucontext.h>
#include "queue.h"
#include <stdlib.h>


STAILQ_HEAD(Thread_Queue, thread_);

struct Thread_Queue* thread_queue;
struct Thread_Queue* queue_to_free;
thread_t current_thread;
//thread_t thread_to_free;
thread_t old_thread;

// Initialise la thread_queue
extern void init_thread_queue()
{
  STAILQ_INIT(thread_queue);
}

extern void init_queue_to_free(){
  STAILQ_INIT(queue_to_free);
}



extern thread_t thread_self(){
  return current_thread;
}

extern int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){
  thread_t t = malloc(sizeof(thread_s));

  getcontext(&(t->context));
  (t->context).uc_stack.ss_size=64*1024;
  (t->context).uc_stack.ss_sp = malloc(t->context.uc_stack.ss_size);
  (t->context).uc_stack.ss_flags = 0;
  (t->context).uc_link=NULL;
  makecontext(&(t->context), (void (*)(void))func, 1, funcarg);
  (t->status) = READY;

  *newthread = t;

  STAILQ_INSERT_TAIL(thread_queue, t, next);

  return 0;
}


extern void thread_yield(void){
  current_thread->status = WAITING;
  old_thread = current_thread;

  if(thread_queue->stqh_first)
  {
      current_thread = thread_queue->stqh_first;
      thread_queue->stqh_first = thread_queue->stqh_first->next.stqe_next;
  }

  current_thread->status = RUNNING;
  swapcontext(&(old_thread->context), &(current_thread->context));

}

extern int thread_join(thread_t thread, void **retval){
  
  //On le met en attente
  current_thread->status= WAITING;
  //On met le thread en running si ce n'est déja fait
  thread->status=RUNNING;
  //On déclare que le prochain à exécuter après thread est current_thread
  thread->next.stqe_next = current_thread;

  if(retval!=NULL){
    *retval=thread->retval;
  }

  free((thread->context).uc_stack.ss_sp);
  free(thread);
  if(!retval)
      return 0;
  return -1;
}


extern void thread_exit(void *retval) __attribute__ ((__noreturn__)){
  current_thread->status= FINISHED;
  thread_t tmp_t= current_thread;
  current_thread=current_thread->next;
  current_thread->status=READY;
  STAILQ_INSERT_TAIL(queue_to_free,tmp_t,next);
  //Supprime t'on la tete de la queue? ou garde t'on tous les threads termines?
  setcontext((*thread_queue)->context);
};
