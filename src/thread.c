#include "thread.h"


extern thread_t thread_self(void){


}

extern int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){

}

extern int thread_yield(void){

}

extern int thread_join(thread_t thread, void **retval){


}


extern void thread_exit(void *retval) __attribute__ ((__noreturn__)){


}
