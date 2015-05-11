#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/time.h>

#include "thread.h"
#include "thread_queue.h"
#include "scheduler.h"

#define PERIOD 1000

struct sigaction preemption_sigalarme;
int tour=0;

/*
	TODO
	Ameliorations
*/ 

/** Variables globales **/
struct queue runqueue = {.head = {{NULL, NULL}}, .num_elmt = -1};
void * stack_free; // dernière pile de contextes à libérer
struct thread * _runningthread;
static struct thread threadmain; // évite les fuite mémoire

/** Maccro Debuggage **/
#define PRT(X) {fprintf(stderr,#X); fprintf(stderr," running thread is %p \n",_runningthread);}

/** Fonctions privées **/

/* Libération du dernier thread et de sa pile allouée dynamiquement */
void free_func()
{
	if (stack_free != NULL)
	{
		free(stack_free);
		free(_runningthread);
		exit(0); //?? : lorsque cette fonction est appelée c'est que le dernier
		// thread termine et qu'il a repassé la main au thread main, qui lui
		// avait déjà terminé, pour que celui ci nettoie le dernier thread.
		// Il faut donc terminer par un exit.
	}
} 

/* Affichage de la queue */
void print_queue(char * str)
{
	int i=0;
	struct thread *prev = queue_top(&runqueue);
	printf("%s\n",str);
	printf("---------[%d]------------\n", runqueue.num_elmt);
	for (i=0; i<runqueue.num_elmt; i++)
	{
		struct thread *next = NULL;
		if (prev)
			printf("(%d) %p\n",i,prev);
		else
			printf("(%d) Unknown thread\n", i);
	next = list_next(&(runqueue.head), prev, list);
	prev = next;
	}
	puts("------------------------");
}


/* AMELIORATION Préemption */

void preemption()
{
	//~ printf("PRIORITY: %d\n",((struct thread*)thread_self())->priority);
	tour ++;
	//~ printf("tour: %d\n",tour);
	if(tour>=((struct thread*)thread_self())->priority)
	{
		tour=0;
		thread_yield();
	}
}

void ignore_sig()
{
}

/* Appel de désactivation des signaux */
void signals_off()
{
	preemption_sigalarme.sa_handler = ignore_sig;   
	sigaction(SIGALRM, &preemption_sigalarme, NULL); 
}

/* Appel de réactivation de la gestion des signaux */
void signals_on()
{
	preemption_sigalarme.sa_handler = preemption;
	sigaction(SIGALRM, &preemption_sigalarme, NULL);
}

/* Enrobe les fonctions pour intercaler le thread_exit */
void return_wraper(void *(*func)(void *), void *funcarg)
{
	void * returnvalue=func(funcarg);
	thread_exit(returnvalue);
}

/* Initialisation de l'environnement des threads si besoin */
void initialisation()
{
	if(runqueue.num_elmt == -1)
	{
		stack_free=NULL;
		/* Initialisation de la liste runqueue */
		queue_init(&runqueue);
		/* Initialisation des valeurs du threadmain*/
		threadmain.finished=0;
		threadmain.retval=NULL;
		threadmain.valgrind_stackid=-1;
		threadmain.waited_by = NULL;
		threadmain.u.uc_stack.ss_sp = NULL;

		/* Le threadmain devient le thread courant */
		_runningthread=&threadmain;

		/* Initialisation de SIGALRM pour la préemption */
		signals_on();
		ualarm(PERIOD, PERIOD);
	}
}


void initialize_context(struct thread * newthread, void *(*func)(void *), void *funcarg)
{
	assert(newthread);
	/* Le contexte du nouveau thread est rempli avec le contexte courant */
	getcontext(&(newthread->u)); 
	/* Allocation de la pile du contexte */
	newthread->u.uc_stack.ss_size = 1024*64;
	newthread->u.uc_stack.ss_sp = NULL;
	newthread->u.uc_stack.ss_sp = malloc(newthread->u.uc_stack.ss_size);
	if ((newthread->u.uc_stack.ss_sp)==NULL)
	{
		perror("initialize_stack error"); 
		return;
	}
	/* Valgrind: juste après l'allocation de la pile */
	newthread->valgrind_stackid = VALGRIND_STACK_REGISTER(
	newthread->u.uc_stack.ss_sp,
	newthread->u.uc_stack.ss_sp 
	+ newthread->u.uc_stack.ss_size);
	/* On lie le contexte à la sentinelle de fin */ 
	newthread->u.uc_link=NULL; 
	makecontext(&(newthread->u),(void (*)(void))return_wraper, 2, func,funcarg);
}

/* Libération de la pile d'un thread */
void free_context_stack(thread_t thread)
{
	assert(thread);
	if(((struct thread*)thread)->valgrind_stackid!=-1)
	{
		assert(((struct thread *)thread)->u.uc_stack.ss_sp);
		VALGRIND_STACK_DEREGISTER(((struct thread*)thread)->valgrind_stackid);
		free(((struct thread *)thread)->u.uc_stack.ss_sp);
	}
}


/** Retourne le thread courant.
 * Initialise la runqueue si elle ne l'a pas été.
 * Ne renvoie jamais d'erreur.
 **/
thread_t thread_self()
{
	initialisation();
	return (void *)_runningthread;
}

/** Crée un nouveau thread et lui laisse la main
 * Initialise la runqueue si elle ne l'a pas été.
 **/
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg, ...)
{
	va_list ap; // Arguments supplémentaires pour founir priorité si besoin
	va_start(ap, funcarg);
	assert(newthread);  
	initialisation();
	
	/* Allocation */
	*newthread=NULL;
	*newthread=(struct thread*)malloc(sizeof(struct thread));
	if(!(*newthread))
	{ // Échec du malloc
		perror("thread_create");
		return -1;
	}

	/* Initialisation de la pile du contexte du nouveau thread
		 et création du contexte */
	initialize_context((struct thread *)*newthread,func,funcarg);
	/* Initialisation des autres champs du nouveau thread */
	((struct thread *)*newthread)->finished = 0;
	((struct thread *)*newthread)->retval = NULL;
	((struct thread *)*newthread)->waited_by = NULL;
	
	/* Mise à jour du status */
	((struct thread *)*newthread)->state = NEW;
	
	/* Mise à jour de la priorité si fournie en argument */
	int p = va_arg(ap, int);
	if(p <= 20)
	{
		((struct thread *)*newthread)->priority = p;
	}
	else
	{
		((struct thread *)*newthread)->priority = 1;
	}
	
	queue_unlink_thread((struct thread *)*newthread);
	
	struct thread * old_running_t = _runningthread;
	_runningthread = next_running_t_create(SCHED_BASIC, &runqueue, ((struct thread *)*newthread), old_running_t);
	
	/* Swap du contexte courant vers le contexte du nouveau thread */
	swapcontext(&(old_running_t->u), &(_runningthread->u));

	return 0;
}


/** Passe la main à un autre thread de la runqueue.
 * Version courante : la main est passée au premier thread
 * de la runqueue, et le thread courant est placée en fin
 * de runqueue.
 * Initialise la runqueue si elle ne l'a pas été.
 * Ne renvoie jamais d'erreur.
 **/
int thread_yield(void)
{
	signals_off();
	initialisation();
	
	/* Si la queue d'attente est vide, fin */
	if (is_empty(&runqueue))
	{
		return 0;
	}
	
	struct thread * old_running_t = _runningthread;
	_runningthread = next_running_t_yield(SCHED_BASIC, &runqueue,  _runningthread);
	
	/* Swap du contexte courant vers le contexte du nouveau thread */
	swapcontext(&(old_running_t->u), &(_runningthread->u));
	
	signals_on();
	return 0; 
}

/** Attend la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 * Retourne 0 si succès, -1 si erreur
 */
int thread_join(thread_t thread, void **retval)
{ 
	signals_off();
	
	/* Erreur: thread inconnu */
	if (thread == NULL)
	{
		fputs("thread_join: Unknown thread\n", stderr);
		return -1;
	}
	/* Erreur : join sur lui-même */  
	if (thread == thread_self())
	{
		fputs("thread_join: Join on calling thread\n", stderr);
		return -1;
	}
	/* Erreur : join bouclant */  
	if (((struct thread *)thread_self())->waited_by == ((struct thread *)thread))
	{
		fputs("thread_join: Deadlock detected\n", stderr);
		return -1;
	}
	/* Erreur : un autre thread attend de faire un join sur le thread  */
	if (((struct thread *)thread)->waited_by != NULL)
	{
		fputs("thread_join: Another thread is already waiting to join with this thread\n", stderr);
		return -1;
	}

	/* Si le thread attendu n'a pas terminé */
	if(((struct thread *)thread)->finished==0)
	{
		/* Mise à jour de waited_by, puis laisse la main au thread que l'on va attendre */
		((struct thread *)thread)->waited_by = _runningthread;
		/* Rqe : si le thread attendu a fini avant
			 le join, son champ waited_by
			 n'est pas mis à jour. 
		*/

		queue_remove_elt(&runqueue,(struct thread*) thread);

		struct thread * old_running_t =_runningthread;
		_runningthread= next_running_t_join(SCHED_BASIC, &runqueue, old_running_t, (struct thread* )thread);
		
		/* Swap de contexte */
		swapcontext(&(old_running_t ->u),&(_runningthread->u));

	}
	else
	{	/* Sinon ne pas donner la main inutilement */
	}

	/*Tant que le thread qu'on attend n'a pas terminé :
	Redonner la main à l'ordonanceur et ne pas se mettre dans la runqueue.*/
	while(((struct thread *)thread)->finished==0)
	{
		struct thread * old_running=_runningthread;
		_runningthread=(struct thread* )queue_pop(&runqueue);
		swapcontext(&(old_running->u),&(_runningthread->u));
	}
	
	/* Capture de la valeur de retour du thread attendu si demandée */
	if (retval != NULL)
		*retval=((struct thread *)thread)->retval;
	else
	{ /*Si la valeur de retval == NULL la valeur de retour n'est pas capturée*/
	}
	/* Libération du thread attendu et de son contexte */
	if(((struct thread *)thread)!=&threadmain) // sauf threadmain
	{
		free_context_stack((struct thread*)thread);
		free((struct thread*)thread);
	}
	
	signals_on();
	return 0;
}

/** Termine le thread courant en renvoyant la valeur de retour retval.
 * Ne retourne jamais.
 **/
void thread_exit(void *retval)
{
	signals_off();
	assert(_runningthread);
	_runningthread->finished=1;
	_runningthread->retval=retval;
	struct thread * old_running=NULL;
		
	// Si le thread courant est attendu
	if(_runningthread->waited_by)
	{
		// Nouveau thread courant = thread qui attendait
		old_running=_runningthread;
		_runningthread=_runningthread->waited_by;
		// Nettoyage pour un thread différent du threadmain
		if(&threadmain != old_running) 
		{
				/* /!\ Si on est attendu c'est le thread qui a fait le join qui s'occupe de la libération/!\ */
				setcontext(&(_runningthread->u));
		}
		else
		{
			// pas de nettoyage pour le thread main
			// ICI : cas du thread courant qui attend le threadmain
			swapcontext(&(threadmain.u),&(_runningthread->u));
			free_func();
		}
	}
	// Si personne n'attend le thread courant
	// Ou qu'il se termine avant que le join eut été fait
	else
	{
		if (is_empty(&runqueue)) // et que la runqueue est vide
		{
			if(&threadmain != _runningthread)
			{
				// Sauvegarde pour libération postérieure
				stack_free=_runningthread->u.uc_stack.ss_sp;
				// La libération sera faite dans le threadmain
				setcontext(&(threadmain.u));
			}
			else // threadmain
			{
				exit(0);
			}
		}
		else // La queue n'est pas vide
		{
			// Nouveau thread courant = thread en tete de runqueue
			old_running=_runningthread;
			_runningthread=queue_pop(&runqueue);
			if(&threadmain != old_running) 
			{ /* ancien thread courant différent du threadmain pas libéré car il 
			  peut etre join plus tard et si jamais join ?*/
				setcontext(&(_runningthread->u));
			}
			else // on termine le threadmain
			{
				swapcontext(&(threadmain.u),&(_runningthread->u));
				free_func();
			}
		}
	}
	signals_on();
}

