#include "thread.h"
#include<sys/queue.h>
#include<ucontext.h>
extern thread_t thread_self(){
  
}

extern int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){
  thread_t *t = new_thread();
  
  getcontext(&(*newthread)->context);
  ((*newthread)->context)->uc_stack.ss_size=64*1024;
  ((*newthread)->context)->uc_stack.ss_sp=malloc(t->uc_stack.ss_size);
  ((*newthread)->context)->uc_stack.ss_flags=O;
  ((*newthread)->context)->uc_link=NULL;
  makecontext(&(t->context), func, 1, funcarg);
  //ucontext_t current_context;
  //swapcontext(&current_context, &(t->context));
  //return thread_yield();
  
  return 0;
}

extern int thread_yield(void){

}

extern int thread_join(thread_t thread, void **retval){
  //  wait();

  free(((*thread_to_free)->context)->uc_stack.ss_sp);
  free(*thread_to_free);
  return &retval;
}


extern void thread_exit(void *retval) __attribute__ ((__noreturn__)){
  setcontext((*thread_queue)->context);
}
