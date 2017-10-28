#include <ucontext.h> // struct used ucontext_t
#include <stdbool.h>

struct cgoroutine_info {
	/* cgoroutine information */
	size_t id;	
	bool running;
	ucontext_t current;
	
	/* For yield feature */
	bool is_yield_set;
	void * yield_ret;
	ucontext_t ctx_yield_parent;
	
};

struct cgoroutine {
	void (*fn)(struct cgoroutine_info*, void*) ;
	void *argv;
	struct cgoroutine_info info;
};

int cgoroutine_init(int max);
struct cgoroutine * cgoroutine_create(void  ((*fn_thread)(struct cgoroutine_info*, void*)), void * argv);
void cgoroutine_yield(struct cgoroutine_info* info, void * ret_val);
void * cgoroutine_next(struct cgoroutine * cgo);

/* define yield function */
#ifndef yield
#define yield(x) cgoroutine_yield(__cgo__info, (void * ) x)
#endif

#ifndef async
#define async(...) (struct cgoroutine_info* __cgo__info, __VA_ARGS__)
#endif

#ifndef go
#define go cgoroutine_create
#endif

#ifndef next
#define next cgoroutine_next
#endif
