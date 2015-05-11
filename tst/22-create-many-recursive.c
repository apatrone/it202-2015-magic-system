#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "thread.h"
#include <sys/time.h>

/* test de plein de create-destroy récursif.
 *
 * valgrind doit etre content.
 * la durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 * jusqu'à combien de threads cela fonctionne-t-il ?
 *
 * support nécessaire:
 * - thread_create()
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static void * thfunc(void *_nb)
{
  unsigned long nb = (unsigned long) _nb;
  if ((unsigned long) nb > 0) {
    thread_t th;
    int err;
    void *res;
    err = thread_create(&th, thfunc, _nb-1);
    assert(!err);
    err = thread_join(th, &res);
    assert(!err);
    assert(res == _nb-1);
  }
  return _nb;
}

int main(int argc, char *argv[])
{
  unsigned long nb;

  struct timeval start, end;
  gettimeofday(&start, NULL);

  if (argc < 2) {
    printf("argument manquant: nombre de threads\n");
    return -1;
  }

  nb = atoi(argv[1]);

  thfunc((void*) nb);

  printf("%ld threads créés et détruits récursivement\n", nb);
  gettimeofday(&end, NULL);

  printf("time: %ld µs\n", ((end.tv_sec * 1000000 + end.tv_usec)
  - (start.tv_sec * 1000000 + start.tv_usec)));

  return 0;
}
