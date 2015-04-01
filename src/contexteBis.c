#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h> /* ne compile pas avec -std=c89 ou -std=c99 */

#define MAX 100

void f(int i,ucontext_t uc, ucontext_t previous){
  if(i<MAX){
    printf("Contexte %d crée\n",i);
    getcontext(&uc);
      uc.uc_stack.ss_size = 64*1024;
      uc.uc_stack.ss_sp = malloc(uc.uc_stack.ss_size);
      uc.uc_stack.ss_flags=0;
      uc.uc_link = &previous;      
      ++i;
      f(i, uc, previous);
  }
  else printf("Terminé\n");
}




int main(){

  ucontext_t uc, previous;
  f(0, uc, previous);

  return 0;

}
