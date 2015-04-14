#include "thread.h"
#include <sys/queue.h>
#include <ucontext.h>
#include "queue.h"
#include <stdlib.h>


STAILQ_HEAD(Thread_Queue, thread_);

struct Thread_Queue* thread_queue;
thread_t thread_current;
thread_t thread_to_free;
thread_t old_thread;

// Initialise la thread_queue
extern void init_thread_queue()
{
  STAILQ_INIT(thread_queue);
}




extern thread_t thread_self(){
  return thread_current;
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

  //ucontext_t current_context;
  //swapcontext(&current_context, &(t->context));
  //return thread_yield();
  STAILQ_INSERT_TAIL(thread_queue, t, next);

  return 0;
}


extern void thread_yield(void){
  thread_current->status = WAITING;
  old_thread = thread_current;
  //Tester si la queue est vide si non:
  //prendre la tete de la queue et la mettre dans thread_current
  //Supprimer la tete et placer a tete au suivant de la queue
  if(thread_queue->stqh_first)
  {
      thread_current = thread_queue->stqh_first;
      thread_queue->stqh_first = thread_queue->stqh_first->next.stqe_next;
  }

  thread_current->status = RUNNING;
  swapcontext(&(old_thread->context), &(thread_current->context));

}

extern int thread_join(thread_t thread, void **retval){
  //  wait();

  //on déclare qu ele thread dont on attend la fin de l'éxécution est thread
  //thread_current->previous = thread;
  //On le met en attente
  thread_current->status= WAITING;
  //On met le thread en running si ce n'est déja fait
  thread->status=RUNNING;
  //On déclare que le prochain à exécuter après thread est thread_current
  thread->next.stqe_next = thread_current;

  if(retval!=NULL){
    *retval=thread->retval;
  }

  free((thread_to_free->context).uc_stack.ss_sp);
  free(thread_to_free);
  if(!retval)
      return 0;
  return -1;
}


extern void thread_exit(void *retval) __attribute__ ((__noreturn__)){
  thread_to_free=thread_current;
  thread_to_free->next=NULL;
  thread_to_free->status= FINISHED;
  thread_current=thread_current->next;
  setcontext((*thread_queue)->context);
};
