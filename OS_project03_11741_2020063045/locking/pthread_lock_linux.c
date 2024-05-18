#include <stdio.h>
#include <pthread.h>

int shared_resource = 0;

#define NUM_ITERS 10000
#define NUM_THREADS 30000

void lock();
void unlock();

int key = 0;

int compare_and_swap(int* ptr, int oldval, int newval) {
    int ret;
    #if defined(__x86_64__)
    __asm__ __volatile__ (
        "lock cmpxchg %2, %1\n"
        : "=a"(ret), "+m"(*ptr)
        : "r"(newval), "0"(oldval)
        : "memory"
    );
    #elif defined(__aarch64__)
    __asm__ __volatile__ (
        "ldaxr %w0, [%1]\n"
        "cmp %w0, %w2\n"
        "b.ne 1f\n"
        "stlxr %w0, %w3, [%1]\n"
        "1:"
        : "=&r"(ret), "+r"(ptr)
        : "r"(oldval), "r"(newval)
        : "memory"
    );
    #elif
    #error "Unsupported architecture"
    #endif

    return ret;
}

void lock()
{
    while(compare_and_swap(&key, 0, 1) == 1);
}

void unlock()
{  
    key = 0;
}

void* thread_func(void* arg) {
    int tid = *(int*)arg;
    
    lock();
    // printf("Thread %d locked\n", tid);
        for(int i = 0; i < NUM_ITERS; i++)    shared_resource++;
    // printf("Thread %d unlocked\n", tid);
    unlock();
    pthread_exit(NULL);
}

int main() {
    int n = NUM_THREADS;
    pthread_t threads[n];
    int tids[n];
    
    for (int i = 0; i < n; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }
    
    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("expected: %d\n", NUM_THREADS * NUM_ITERS);
    printf("result: %d\n", shared_resource);
    
    return 0;
}