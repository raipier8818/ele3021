typedef uint thread_t;

int _thread_create(thread_t *thread, void*(*start_routine)(void*), void *arg);
int _thread_join(thread_t thread, void **retval);
void _thread_exit(void *retval);