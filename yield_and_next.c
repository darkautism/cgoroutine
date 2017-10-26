#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cgoroutine.h"

static long long cn_allexec=0;

void myfunction(struct cgoroutine_info* info, void * argv) {
	size_t times = 0;

	while(1) {
		__sync_fetch_and_add(&cn_allexec, 1);
		
		// yield this data to main process, if main process do not receive this variable,
		// This function will blocking
		yield(info, (void *)times++);
	}
}

void my_exit(int sig){ // can be called asynchronously
	printf("\nAll run times: %d\n", cn_allexec);
	exit(0);
}

int main() {
	struct cgoroutine *cgos[8];
	size_t times;
	signal(SIGINT, my_exit); 
	
	// Allocate 3 thread as worker
	cgoroutine_init(3);
	
	// Allocate 8 cgoroutine
	for ( size_t i = 0 ; i < 8 ; i++ ) {
		cgos[i] = cgoroutine_create(myfunction, (void *)i);
	}
	size_t cgoroutine_id = 0;
	while (1) {
		// Then, user can use next function to retrieve cgoroutine's value
		times = (size_t) next(cgos[cgoroutine_id++]);
		printf("[%zu]cgoroutine return %zu.\n", cgoroutine_id, times);
		if(cgoroutine_id==8) cgoroutine_id=0;
	}
}
	
