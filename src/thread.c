#include "thread.h"
#include <sys/queue.h>
#include <ucontext.h>
#include "queue.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include "valgrind.h"
#include <sys/time.h>

#include "preemption1.c"

#define PRIORITY 0

STAILQ_HEAD(threadqueue, thread_);

/*
struct itimerval timeslice = {{1,1000000},{1,1000000}};
struct itimerval timer = setitimer(0, timeslice, NULL);
*/

int thread_init = 0;

struct threadqueue thread_queue;
struct threadqueue queue_to_free;
thread_t current_thread;
thread_t old_thread;
thread_t main_thread;

// Initialise la thread_queue

void init_thread(thread_t *t){
  thread_s *thd = malloc(sizeof(thread_s));

  getcontext(&(thd->context));
  (thd->context).uc_stack.ss_size=64*1024;
  (thd->context).uc_stack.ss_sp = malloc((thd->context).uc_stack.ss_size);
  thd->valgrind_stack_id = VALGRIND_STACK_REGISTER((thd->context).uc_stack.ss_sp,(thd->context).uc_stack.ss_sp + (thd->context).uc_stack.ss_size);
  (thd->context).uc_stack.ss_flags = 0;
  (thd->context).uc_link = NULL;
  thd->waiting_by=NULL;
  thd->retval=NULL;
  thd->priority=0;
  *t = thd;

  preemption_init();
  enable_preempt();
}

void init_thread_main(){
  //  printf("thread_init_main called !!!!!\n ");
  main_thread = malloc(sizeof(thread_s));
  getcontext(&(main_thread->context));
  main_thread->retval = NULL;
  main_thread->status = READY;
  main_thread->waiting_by = NULL;



}

void libthread_init() {	  //pas besoin d'allouer la pile pour le main thread car elle est déjà allouée
  STAILQ_INIT(&thread_queue);
  STAILQ_INIT(&queue_to_free);
  init_thread_main(&main_thread);
  getcontext(&(main_thread->context));
  current_thread = main_thread;
  thread_init = 1;


  //activation de la preemption

  preemption_init();
  enable_preempt();
}


static void wrap(void *(*func)(void *),void *funcarg) {
	void *retval;
	retval =func(funcarg);
	thread_exit(retval);
}


thread_t thread_self(){
  if(!thread_init){
    libthread_init();
  }

  return current_thread;
}


void insertion( thread_t t){
  //      printf("ici ok\n");
  int i=t->priority;
  thread_t tmp;
  tmp=STAILQ_FIRST(&thread_queue);
  if(!(tmp)){
    STAILQ_INSERT_HEAD(&thread_queue, t, next);}
  else  if(i>((tmp)->priority)){
    STAILQ_INSERT_HEAD(&thread_queue, t, next);
  }
  else{
    while((i<=(tmp)->priority)&&(STAILQ_NEXT(tmp, next)!=NULL)){
      tmp= STAILQ_NEXT(tmp, next);
    }
    STAILQ_INSERT_AFTER(&thread_queue, tmp, t, next);
    // free((tmp->context).uc_stack.ss_sp);
    //free(tmp);
  }
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){

  disable_preempt();
  if(!thread_init){
    libthread_init();
  }

  thread_t t;
  init_thread(&t);

  makecontext(&(t->context), (void (*)(void))wrap, 2, func, funcarg);
  (t->status) = READY;

  *newthread = t;
  insertion(t);// STAILQ_INSERT_TAIL(&thread_queue, t, next);

  enable_preempt();
  return 0;
}


int thread_yield(void){
  disable_preempt();
  if(!thread_init){
    libthread_init();
  }

  old_thread = current_thread;

  if(STAILQ_FIRST(&thread_queue)!=NULL)
    {
      current_thread = STAILQ_FIRST(&thread_queue);
      STAILQ_REMOVE_HEAD(&thread_queue, next);
    }

  //current_thread->status = RUNNING;
  if (old_thread != current_thread ) {
    insertion(old_thread); /* STAILQ_INSERT_TAIL(&thread_queue, old_thread, next);*/
    //setitimer(0,timeslice, timer);
    int ret= swapcontext(&(old_thread->context), &(current_thread->context));
    if(ret==-1)
      return -1;

  }
  enable_preempt();
  return 0;

}

int thread_join(thread_t thread, void **retval)
{
  //  printf("thread_init %i\n", thread_init);
  disable_preempt();
  assert(thread_init);

  // Est-ce qu'il est attendu par quelqu'un d'autre ? (si oui on quitte)
  if(thread->waiting_by != NULL){
    return -1;
  }
  
  // Si non, c'est nous qui l'attendons et personne d'autre
  else {
    current_thread->status = WAITING;
    thread->waiting_by = current_thread;
  }

  // Est-ce que le thread attendu est fini ?
  if(thread->status != FINISHED) {
    old_thread = current_thread;

    // Si le thread est ready, on lui donne la main pour finir au plus tot
    if (thread->status == READY) {
      STAILQ_REMOVE( &thread_queue, thread, thread_, next );
      current_thread = thread;
    }
    // Sinon on donne la main a un autre
    else {
      if(STAILQ_FIRST(&thread_queue)){
        current_thread = STAILQ_FIRST(&thread_queue);
        STAILQ_REMOVE_HEAD(&thread_queue, next);
      }//pas de else?
    }
    assert( old_thread != current_thread );
    //setitimer(0,timeslice ,timer );
    swapcontext(&(old_thread->context), &(current_thread->context));
  }

  // Normalement on ne reviens ici que si le thread est fini
  //  printf("thread->status: %d\n", (int)thread->status);
  assert( thread->status == FINISHED );

  // Recupere la valeur de retour
  if(retval!=NULL){
    *retval = thread->retval;
  }

  // On libere les ressources allouées a thread
  if(thread != main_thread ){
    VALGRIND_STACK_DEREGISTER(thread->valgrind_stack_id);
    free((thread->context).uc_stack.ss_sp);
  }
  free(thread);
  enable_preempt();
  return 0;
}



void thread_exit(void *retval) {
  disable_preempt();

  assert(thread_init);
  //  printf("exit\n");

  current_thread->retval = retval;
  current_thread->status = FINISHED;

  old_thread = current_thread;

  // Il y a un thread qui m'attend, je lui donne la main
  if(current_thread->waiting_by){
    // fprintf(stderr, "Je donne la main\n");
    current_thread = current_thread->waiting_by;
    current_thread->status = READY;
  }
  // Sinon je donne la main au premier qui est ready
  else if (STAILQ_FIRST(&thread_queue)){
    //    fprintf(stderr, "Je donne la main\n");
    current_thread = STAILQ_FIRST(&thread_queue);
    STAILQ_REMOVE_HEAD(&thread_queue, next);
  }
  /* Plus personne n'est pret, donc normalement plus personne n'existe */
  else {
    exit(0);
  }

  //  printf("current thread= %d\n", current_thread->status);
  assert(current_thread->status == READY);

  int rc = setcontext( &(current_thread->context));
  if(rc==-1)
    perror("Error: setcontext");
}

void clean_finished_thread(void) __attribute__((destructor));
void clean_finished_thread(){
  if(current_thread != main_thread && current_thread != NULL){
    //    printf("on free le thread\n");
    VALGRIND_STACK_DEREGISTER(current_thread->valgrind_stack_id);
    //free((current_thread->context).uc_stack.ss_sp);
  }
  free(current_thread);
  enable_preempt();
}
