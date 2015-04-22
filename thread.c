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
thread_t old_thread;
thread_t main_thread;

// Initialise la thread_queue


extern void init_thread(thread_t *t){
  thread_s *thd = malloc(sizeof(thread_s));

  getcontext(&(thd->context));
  (thd->context).uc_stack.ss_size=64*1024;
  (thd->context).uc_stack.ss_sp = malloc((thd->context).uc_stack.ss_size);
  (thd->context).uc_stack.ss_flags = 0;
  (thd->context).uc_link=NULL;
  thd->waiting_by=NULL;
  thd->retval=NULL;

  *t = thd;
}

void libthread_init() {	  //pas besoin d'allouer la pile pour le main thread car elle est déjà allouée
  STAILQ_INIT(&thread_queue);
  STAILQ_INIT(&queue_to_free);
  //init_thread(&current_thread);
  init_thread(&old_thread);
  init_thread(&main_thread);
  getcontext(&(main_thread->context));
  current_thread=main_thread;
  thread_init = 1;	
}

extern thread_t thread_self(){
  if(!thread_init){
    libthread_init();
  }
  	
  return current_thread;
}

extern int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){

  if(!thread_init){
    libthread_init();
  }

  thread_t t;
  init_thread(&t);
 	
  makecontext(&(t->context), (void (*)(void))func, 1, funcarg); 
  (t->status) = READY;

  *newthread = t;
  STAILQ_INSERT_TAIL(&thread_queue, t, next);

  return 0;
}


extern int thread_yield(void){

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

extern int thread_join(thread_t thread, void **retval){ 


  printf("join\n");
  assert(thread_init);
	
  // Est-ce qu'il est attendu par quelqu'un d'autre ? (si oui on quitte)
  if(thread->waiting_by){
    return -1;
  } 
  // Si non, c'est nous qui l'attendons et personne d'autre
  else {
 	 current_thread->status = WAITING;
 	 thread->waiting_by = current_thread;
  }
  

  // Est-ce que le thread attendu est fini ?
  if(thread->status != FINISHED) {
    // Si le thread est ready, on lui donne la main pour finir au plus tot
    if (thread->status == READY) {
      STAILQ_REMOVE( &thread_queue, thread, thread_, next );
      current_thread = thread;
    }
    // Sinon on donne la main a un autre
    else {
      current_thread = STAILQ_FIRST(&thread_queue);
      STAILQ_REMOVE_HEAD(&thread_queue, next);
    }
    swapcontext(&(thread->context), &(current_thread->context));
  }
	 
  // Normalement on ne reviens ici que si le thread est fini
  assert( thread->status == FINISHED );
 
  // Recupere la valeur de retour
  if(retval!=NULL){
    *retval = thread->retval;
  }	
  
  
  // On libere les ressources allouées a thread
  if(thread != main_thread){
    free((thread->context).uc_stack.ss_sp);
    free(thread);
  }

  return 0;
}



void thread_exit(void *retval) {
  assert(thread_init);
  printf("exit\n");

  current_thread->retval = retval;
  thread_t tmp_t = current_thread; 
  
  // Il y a un thread qui m'attend, je lui donne la main
  if(tmp_t->waiting_by){
    current_thread = tmp_t->waiting_by;
  }    
  // Sinon je donne la main au premier qui est ready
  else{
    current_thread = STAILQ_FIRST(&thread_queue);
    STAILQ_REMOVE_HEAD(&thread_queue, next);
  }

  STAILQ_INSERT_TAIL(&queue_to_free,tmp_t,next);
  tmp_t->status = FINISHED;
  assert(current_thread->status == READY);

  //Supprime t'on la tete de la queue? ou garde t'on tous les threads termines?

  int rc = setcontext(&(current_thread->context)); 
  if(rc==-1)
    perror("Error: setcontext");
}
