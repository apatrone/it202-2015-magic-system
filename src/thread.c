#include "thread.h"
#include <sys/queue.h>
#include <ucontext.h>
#include "queue.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

STAILQ_HEAD(threadqueue, thread_) ;

int thread_init = 0;

struct threadqueue thread_queue;
struct threadqueue queue_to_free;  //à changer sans *
thread_t current_thread;
//thread_t thread_to_free;
thread_t old_thread;
thread_t main_thread;

// Initialise la thread_queue

void init_thread(thread_t *t){
  thread_s *thd = malloc(sizeof(thread_s));
  printf("thread_init called\n");
  getcontext(&(thd->context));
  (thd->context).uc_stack.ss_size=64*1024;
  (thd->context).uc_stack.ss_sp = malloc((thd->context).uc_stack.ss_size);
  (thd->context).uc_stack.ss_flags = 0;
   
  (thd->context).uc_link=NULL;
  thd->retval=NULL;
  *t = thd;
}

void init_thread_main(){
  printf("thread_init_main called !!!!!\n ");
  main_thread = malloc(sizeof(thread_s));
  getcontext(&(main_thread->context));
  main_thread->retval=NULL;
}

void libthread_init() {	  
  STAILQ_INIT(&thread_queue);
  STAILQ_INIT(&queue_to_free);
  init_thread(&current_thread);
  init_thread_main(&main_thread);
  getcontext(&(main_thread->context));
  current_thread=main_thread;
  thread_init = 1;	
}

thread_t thread_self(){
  if(!thread_init){
    libthread_init();
  }
  	
  return current_thread;
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){

  if(!thread_init){
    libthread_init();
  }

  thread_t t;
  init_thread(&t);
 	

  makecontext(&(t->context), (void (*)(void))func, 1, funcarg); 
  ///	t->retval=func(funcarg);
  (t->status) = READY;

  *newthread = t;
  STAILQ_INSERT_TAIL(&thread_queue, t, next); //erreur de seg ici

  return 0;
}


int thread_yield(void){

  if(!thread_init){
    libthread_init();    
  } 	
  current_thread->status = WAITING;
  old_thread = current_thread;

  if((&thread_queue)->stqh_first!=NULL)
    {
      current_thread = (&thread_queue)->stqh_first;
      (&thread_queue)->stqh_first = (&thread_queue)->stqh_first->next.stqe_next;
    }
  else{	
    current_thread = main_thread;
  }
  current_thread->status = RUNNING;
  int ret= swapcontext(&(old_thread->context), &(current_thread->context));
  if(ret==0)
    return 0;
  return -1;

}

int thread_join(thread_t thread, void **retval){
  assert(thread_init);
  //On le met en attente
  current_thread->status= WAITING;	
  //On met le thread en running si ce n'est déja fait
  thread->status=RUNNING;
  //On déclare que le prochain à exécuter après thread est current_thread
  thread->next.stqe_next = current_thread;

	
  printf("retval: %x\n", *retval);
  if(retval!=NULL){
    *retval=thread->retval;
  }	

  if(thread != main_thread){
    clean_finished_thread();
  }

  return 0;
}



void thread_exit(void *retval){
  assert(thread_init);
  printf("exit\n");
  current_thread->status= FINISHED;
  current_thread->retval=retval;
  printf("retval: %x\n", retval);
  printf("->retval: %x\n", current_thread->retval);

  thread_t tmp_t= current_thread;  printf("exit\n");
  current_thread=current_thread->next.stqe_next;  
  if(current_thread){
    current_thread->status=READY;printf("boucle\n");
  }

  STAILQ_INSERT_TAIL(&queue_to_free,tmp_t,next);
  printf("exit\n");
  //Supprime t'on la tete de la queue? ou garde t'on tous les threads termines?

  int rc=setcontext(&((thread_queue.stqh_first)->context)); 
  if(rc==-1)
    perror("Error: setcontext");
}


void kill_main_thread(){
  printf("on free le main \n");
  free(main_thread);
  main_thread = NULL;
}


void clean_finished_thread(){
  printf("on free le thread\n");
  if(current_thread != main_thread && current_thread != NULL){
    if( current_thread->retval != NULL)
      (current_thread->retval) = current_thread->retval;
    //if( (th->context).uc_stack.ss_sp != NULL)
    free((current_thread->context).uc_stack.ss_sp);
    free(current_thread);
  }
}
