#include <pthread.h>

void init_lock_mutex_cond(pthread_mutex_t *mutex, pthread_cond_t *cond);

void wait_for_cb_invoke(pthread_mutex_t *mutex, pthread_cond_t *cond, int *complete);

