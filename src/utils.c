#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void init_lock_mutex_cond(pthread_mutex_t *mutex, pthread_cond_t *cond) {
	if(0 > pthread_cond_init(cond, NULL) ) {
		perror("pthread_cond_init");
		exit(1);
	}
	if(0 > pthread_mutex_init(mutex, NULL) ) {
		perror("pthread_mutex_init");
		exit(1);
	}

	if(0 > pthread_mutex_lock(mutex)) {
		perror("pthread_cond_wait");
		exit(1);
	}
}

void wait_for_cb_invoke(pthread_mutex_t *mutex, pthread_cond_t *cond, int *complete) {
	while(!*complete) {
		if(0 > pthread_cond_wait(cond, mutex))
		{
			perror("pthread_cond_wait");
			exit(1);
		}
	}

	if(0 > pthread_mutex_unlock(mutex)) {
		perror("pthread_cond_wait");
		exit(1);
	}
}
