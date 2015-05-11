#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "thread.h"

#include <sys/time.h>

/* test de plein de create-destroy consécutifs.
 *
 * valgrind doit etre content.
 * la durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 * jusqu'à combien de threads cela fonctionne-t-il ?
 *
 * support nécessaire:
 * - thread_create()
 * - thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static void * thfunc(void *dummy __attribute__((unused)))
{
  thread_exit(NULL);
}

int main(int argc, char *argv[])
{
  thread_t th;
  int err, i, nb;
  void *res;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  if (argc < 2) {
    printf("argument manquant: nombre de threads\n");
    return -1;
  }

  nb = atoi(argv[1]);

  for(i=0; i<nb; i++) {
    err = thread_create(&th, thfunc, NULL);
    assert(!err);
    err = thread_join(th, &res);
    assert(!err);
    assert(res == NULL);
  }

  printf("%d threads créés et détruits\n", nb);

  gettimeofday(&end, NULL);

  printf("time: %ld µs\n", ((end.tv_sec * 1000000 + end.tv_usec)
  - (start.tv_sec * 1000000 + start.tv_usec)));

  return 0;
}
