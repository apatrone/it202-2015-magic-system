#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

long long index1 = 0;

void * loop()
{
  while(1)
    ++index1;
}

int main()
{
  int i = 0;
  thread_t th;
  thread_create(&th, loop, NULL);

  for(i = 0; i < 10; ++i)
    {
      struct timeval start, end;
      gettimeofday(&start, NULL);
      printf("Pas: %lld\n", index1);
      thread_yield();

      gettimeofday(&end, NULL);

      printf("Time: %ld µs\n", ((end.tv_sec * 1000000 + end.tv_usec)
				- (start.tv_sec * 1000000 + start.tv_usec)));

    }
  return 0;
}
