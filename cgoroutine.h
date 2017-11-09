#include <ucontext.h> // struct used ucontext_t
#include <stdbool.h>
#include "cross-platform.h"

struct cgoroutine {
	void (*fn)(struct cgoroutine*, void*) ;
	void *argv;
	/* cgoroutine information */
	size_t id;
	bool run;
	bool finished;
	void * ret_val;
	struct cgoroutine* volatile await_for;
	ucontext_t current;
	
	/* For yield feature */
	bool is_yield_set;
	void * volatile yield_ret;
	ucontext_t ctx_yield_parent;
};

int cgoroutine_init(int max);
struct cgoroutine * cgoroutine_create(void  ((*fn_thread)(struct cgoroutine*, void*)), void * argv);
void cgoroutine_yield_data_and_wait(struct cgoroutine* info, void * ret_val);
void cgoroutine_yield(struct cgoroutine* info);
void * cgoroutine_next(struct cgoroutine * cgo);
void * cgoroutine_await(struct cgoroutine * cgo, struct cgoroutine * waitfor);
void cgoroutine_free(struct cgoroutine * cgo);

/* define yield function */
#define YIELD_DATA_AND_WAIT(x)	cgoroutine_yield_data_and_wait(__cgo__, (void*)x)

// #define YIELD_ARG_ERR() # error "Please specify build type in the Makefile"

#define YIELD_GET_ARG(arg1, arg2, FUNC, ...) FUNC

#define yield(...) YIELD_GET_ARG(,	##__VA_ARGS__,				\
		YIELD_DATA_AND_WAIT( __VA_ARGS__),						\
		cgoroutine_yield(__cgo__))								\

// #ifndef yield
// #define yield(x) cgoroutine_yield(__cgo__, (void * ) x)
// #endif

#ifndef async
#define async(...) (struct cgoroutine* __cgo__, __VA_ARGS__)
#endif

#ifndef cgo
#define cgo(x,y) cgoroutine_create(x,(void*)y)
#endif

#ifndef cgofree
#define cgofree cgoroutine_free
#endif

#ifndef next
#define next cgoroutine_next
#endif

#ifndef await
#define await(x) cgoroutine_await( __cgo__, x)
#endif

#ifndef cgoreturn
#define cgoreturn(x) __cgo__->ret_val = (void*)x; smb();return
#endif

// #define cgoreturn 	__asm volatile( "mov %0, 8(%%rbx)\n" ::"r" (return_code));