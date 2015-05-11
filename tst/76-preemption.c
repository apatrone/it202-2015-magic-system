#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

long long index = 0;

void * loop(void * unused)
{
  while(1)
    ++glob;
}

int main()
{
  int i = 0;
  thread_t th;
  thread_create(&th, loop, NULL);

	for(i = 0; i < 10; ++i)
  {
    printf("Pas: %lld\n", index);
    thread_yield();
  }
  return 0;
}
