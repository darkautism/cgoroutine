#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cgoroutine.h"

static long long cn_allexec=0;

void myfunction async(void * argv) {
	long long times = 0;

	while(1) {
		__sync_fetch_and_add(&cn_allexec, 1);
		
		// yield this data to main process, if main process do not receive this variable,
		// This function will blocking
		yield(times++);
	}
}

void my_exit(int sig){ // can be called asynchronously
	printf("\nAll run times: %d\n", cn_allexec);
	exit(0);
}

int main() {
	struct cgoroutine *cgos[8];
	long long times;
	signal(SIGINT, my_exit); 
	
	// Allocate 3 thread as worker
	cgoroutine_init(3);
	
	// Allocate 8 cgoroutine
	for ( long long i = 0 ; i < 8 ; i++ ) {
		// cgos[i] = go cgoroutine_create(myfunction, (void *)i);
		cgos[i] = cgo( myfunction, (void *)i);
	}
	
	long long cgoroutine_id = 0;
	
	while (1) {
		// Then, user can use next function to retrieve cgoroutine's value
		times = (long long) next(cgos[cgoroutine_id]);
		if (times != -1) { // success
			printf("[%zu]cgoroutine return %zu.\n", cgoroutine_id, times);
			if(++cgoroutine_id==8) cgoroutine_id=0;
		}
	}
}
	
