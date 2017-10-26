#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <lfq.h>
#include <sys/time.h>
#define __USE_GNU
#include <signal.h>
#include "cgoroutine.h"


#ifndef thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define thread_local __declspec(thread) 
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __xlC__
#  define thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#endif

#ifdef CGOROUTINE_SIGNAL
#define CGOROUTINE_TICKS 50000
#endif


// TODO: These variable should store in a struct when we wanna support two group cgoroutine
static pthread_t  * cgoroutine_thread_hole;
static bool is_cgoroutine_running = true;
static size_t cn_cgoroutine = 0;
static struct lfq_ctx * ctx;
static thread_local size_t threadid = 0;

#ifdef CGOROUTINE_SIGNAL
void sigroutine(int signo) {
	switch (signo){
		case SIGALRM:
			printf("[%ld] Catch a signal -- SIGALRM \n", threadid);
			signal(SIGALRM, sigroutine);
			break;
		case SIGVTALRM:
			printf("[%ld] Catch a signal -- SIGVTALRM \n", threadid);
			signal(SIGVTALRM, sigroutine);
			break;
		case SIGPROF:
			printf("[%ld] Catch a signal -- SIGPROF \n", threadid);
			signal(SIGPROF, sigroutine);
			break;
			
	}
	return;
}

void myhandle(int mysignal, siginfo_t *si, void* arg)
{    
	ucontext_t *context = (ucontext_t *)arg;
	printf("Address from where crash happen is %x \n",context->uc_mcontext.gregs[REG_RIP]);
	// context->uc_mcontext.gregs[REG_RIP] = context->uc_mcontext.gregs[REG_RIP] + 0x04 ;
}
#endif

void * cgoroutine_thread_manager(void * argv) {
	threadid = (size_t) argv;
	struct cgoroutine * cgo;
	ucontext_t ctx_function_return;
#ifdef CGOROUTINE_SIGNAL
	// struct itimerval timer, ignore;
	// /* Set timer */
	// timer.it_value.tv_sec = 0;
	// timer.it_value.tv_usec = CGOROUTINE_TICKS;
	// timer.it_interval.tv_sec = 0;
	// timer.it_interval.tv_usec = CGOROUTINE_TICKS;
	// setitimer(ITIMER_PROF , &timer, &ignore);
	
	// struct sigaction action;
	// action.sa_sigaction = &myhandle;
	// action.sa_flags = SA_SIGINFO;
	// sigaction(SIGPROF,&action,NULL);
	// // signal(SIGPROF, sigroutine);
#endif

	while (is_cgoroutine_running) {
		/* watch queue is there any cgoroutine */
		if (ctx->count==0) {
			usleep(100);
			continue;
		}
		cgo = lfq_dequeue(ctx);
		if (cgo) {
			getcontext(&ctx_function_return);
			if (cgo->info.current.uc_link != &ctx_function_return ) {
				cgo->info.current.uc_link = &ctx_function_return;
				if ( __sync_bool_compare_and_swap( &cgo->info.running, false, true ) ) {
					makecontext( &cgo->info.current, (void (*) (void)) cgo->fn, 2, (void *)&cgo->info, (void *)cgo->argv, 0);
				} 
				
				swapcontext(&cgo->info.ctx_yield_parent, &cgo->info.current);
				// cgoroutine swap cpu here, push back in queue
				cgo->info.current.uc_link = 0;
				lfq_enqueue(ctx, (void*)cgo);
			} else { // cgoroutine return here, free cgoroutine
				cgo->info.current.uc_link = 0;
				free(cgo->info.current.uc_stack.ss_sp);
				free(cgo);
			}
		}
		
	}
}

// TODO: add check and return error no
int cgoroutine_init(int max) {
	cgoroutine_thread_hole = malloc(sizeof(pthread_t )*max);
	ctx = calloc(sizeof(ctx),1);
	lfq_init(ctx);
	for ( size_t i = 0 ; i < max ; i++ ) {
		pthread_create(&cgoroutine_thread_hole[i], NULL, cgoroutine_thread_manager, (void * ) i);
	}
}

// TODO: add check and return error no (calloc/lfq_enqueue)
struct cgoroutine * cgoroutine_create(void  ((*fn_thread)(struct cgoroutine_info*, void*)), void * argv) {
	struct cgoroutine * cgo = calloc(sizeof(struct cgoroutine),1);
	cgo->fn = fn_thread;
	cgo->argv = argv;
	cgo->info.id = __sync_fetch_and_add(&cn_cgoroutine, 1);
	if ( getcontext(&cgo->info.current) )
		printf("getcontext failed!\n");
	cgo->info.current.uc_stack.ss_sp = malloc(4096);
	cgo->info.current.uc_stack.ss_size  = 4096;
	lfq_enqueue(ctx, (void*)cgo);
	return cgo;
}

void cgoroutine_yield(struct cgoroutine_info* info, void * ret_val) {
	do {
		if( info->is_yield_set ) {
			swapcontext( &info->current, &info->ctx_yield_parent );
		} else {
			if ( __sync_bool_compare_and_swap( &info->is_yield_set, false, true ) ) {
				info->yield_ret = ret_val;
			}
		}
	} while ( info->is_yield_set );
}

void * cgoroutine_next(struct cgoroutine * cgo) {
	if ( __sync_bool_compare_and_swap( &cgo->info.is_yield_set, true, false ) ) {
		return cgo->info.yield_ret;
	}
	return 0;
}
