#include <time.h>
#include <stdio.h>

#include "thread.h"

#define TIMESLICE ((long long) 1e6)

static struct sigevent   sevent;
static struct itimerspec iti;
static        timer_t    timer_id;

int enablestatus = 0;


static void set_timer_preempt (long time_slice)
{
  iti.it_value.tv_sec     = time_slice / 1000000000;
  iti.it_value.tv_nsec    = time_slice % 1000000000;
  //iti.it_interval.tv_sec  = iti.it_value.tv_sec;
  //iti.it_interval.tv_nsec = iti.it_value.tv_nsec;

  timer_settime(timerid, 0, &iti, NULL);
}



