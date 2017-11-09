#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <lfq.h>
#include <sys/time.h>
#define __USE_GNU
#include <signal.h>
#include "cgoroutine.h"
#include "cross-platform.h"


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

void * cgoroutine_thread_manager(void * argv) {
	threadid = (size_t) argv;
	struct cgoroutine * cgo;
	ucontext_t ctx_function_return;
	void * fn_return = 0;
	while (is_cgoroutine_running) {
		/* watch queue is there any cgoroutine */
		if (ctx->count==0) {
			usleep(100);
			continue;
		}
		cgo = lfq_dequeue_tid(ctx, threadid);
		if (cgo) {
			lmb();
			if (cgo->await_for && !cgo->await_for->finished)
				lfq_enqueue(ctx, (void*)cgo);
			
			getcontext(&ctx_function_return);
			if (cgo->current.uc_link != &ctx_function_return ) {
				cgo->current.uc_link = &ctx_function_return;
				if ( __sync_bool_compare_and_swap( &cgo->run, false, true ) ) {
					makecontext( &cgo->current, (void (*) (void)) cgo->fn, 2, (void *)cgo, (void *)cgo->argv, 0);
				}
				
				swapcontext(&cgo->ctx_yield_parent, &cgo->current);
				// cgoroutine swap cpu here, push back in queue
				//printf("yield\n");
				cgo->current.uc_link = 0;
				lfq_enqueue(ctx, (void*)cgo);
			} else { // cgoroutine return here
				//cgo->ret_val = ((void **)cgo->current.uc_stack.ss_sp)[511];
				//printf("function return %d\n", (long long)cgo->ret_val);
				cgo->finished = true;
				smb();
			}
		}
	}
}

// TODO: add check and return error no
int cgoroutine_init(int max) {
	cgoroutine_thread_hole = malloc(sizeof(pthread_t )*max);
	ctx = calloc(sizeof(ctx),1);
	lfq_init(ctx, max);
	for ( size_t i = 0 ; i < max ; i++ ) {
		pthread_create(&cgoroutine_thread_hole[i], NULL, cgoroutine_thread_manager, (void * ) i);
	}
}

// TODO: add check and return error no (calloc/lfq_enqueue)
struct cgoroutine * cgoroutine_create(void  ((*fn_thread)(struct cgoroutine *, void*)), void * argv) {
	struct cgoroutine * cgo = calloc(sizeof(struct cgoroutine),1);
	cgo->fn = fn_thread;
	cgo->argv = argv;
	cgo->id = __sync_fetch_and_add(&cn_cgoroutine, 1);
	if ( getcontext(&cgo->current) )
		printf("getcontext failed!\n");
	cgo->current.uc_stack.ss_sp = malloc(4096);
	cgo->current.uc_stack.ss_size  = 4096;
	lfq_enqueue(ctx, (void*)cgo);
	return cgo;
}

void cgoroutine_yield_data_and_wait(struct cgoroutine * cgo, void * ret_val) {
	do {
		if( cgo->is_yield_set ) {
			swapcontext( &cgo->current, &cgo->ctx_yield_parent );
		} else {
			cgo->yield_ret = ret_val;
			lmb();
			if (! __sync_bool_compare_and_swap( &cgo->is_yield_set, false, true ) ) {
				// should not happened
			}
		}
	} while ( cgo->is_yield_set );
	// printf("good yield\n");
}

void cgoroutine_yield(struct cgoroutine * cgo) {
	swapcontext( &cgo->current, &cgo->ctx_yield_parent );
}

void * cgoroutine_next(struct cgoroutine * cgo) {
	if (cgo->is_yield_set) {
		lmb();
		void * ret = cgo->yield_ret;
		if ( __sync_bool_compare_and_swap( &cgo->is_yield_set, true, false ) ) {
			return ret;
		}
	}
	return (void *)-1;
}

void cgoroutine_free(struct cgoroutine * cgo) {
	cgo->current.uc_link = 0;
	free(cgo->current.uc_stack.ss_sp);
	free(cgo);
}

void * cgoroutine_await(struct cgoroutine * cgo, struct cgoroutine * waitfor) {
	cgo->await_for = waitfor;
	smb();
	cgoroutine_yield(cgo);
	lmb();
	return cgo->ret_val;
}
