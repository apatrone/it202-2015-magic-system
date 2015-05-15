#include <time.h>
//#include <sys/time.h>
#include <stdio.h>

#include "thread.h"


#define TIMESLICE ((long long) 1e6)


static struct itimerspec iti;
static struct sigevent   sevent;
static        timer_t    timer_id;

int enablestatus = 0;


/*
/!\ Il faut absolument protéger les partis de codes ou il y a des swapcontext
==> Solution proposé, activer et desactiver la preemption
*/

static void set_timer_preempt (long time_slice)
{
  iti.it_value.tv_sec     = time_slice / 1000000000;
  iti.it_value.tv_nsec    = time_slice % 1000000000;
  //iti.it_interval.tv_sec  = iti.it_value.tv_sec;
  //iti.it_interval.tv_nsec = iti.it_value.tv_nsec;

  timer_settime(timer_id, 0, &iti, NULL);
}


void enable_preempt(){
  enablestatus = 1;
}

void disable_preempt(){
  enablestatus = 0;
}

void preempt(int signal){
  if (signal == enablestatus){
    //on donne la main, il faudra peut etre modifier thread_yield()
    thread_yield();
  }
}

void preemption_init(){
// init les signaux
// init le timer

struct sigaction sig;
sig.sa_handler = preempt; //pointeur de fonction
sigemptyset(&sig.sa_mask);
sig.sa_flags = 0;
sigaction(SIGALRM, &sig, NULL);

sevent.sigev_notify = SIGEV_SIGNAL;
sevent.sigev_signo = SIGALRM;
sevent.sigev_value.sival_ptr = &timer_id;

timer_create(CLOCK_THREAD_CPUTIME_ID, &sevent, &timer_id);

set_timer_preempt(TIMESLICE);
}

void premption_quit(){
  timer_delete(timer_id);
}
