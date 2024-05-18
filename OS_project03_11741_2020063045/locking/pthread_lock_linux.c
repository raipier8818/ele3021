#include <stdio.h>
#include <pthread.h>

int shared_resource = 0;

#define NUM_ITERS 1000000
#define NUM_THREADS 100

void lock();
void unlock();

int key = 0;

// __sync_lock_test_and_set
int test_and_set(int *ptr, int val)
{
    // x86-64
    asm volatile(
        "xchg %0, %1"               // atomic swap between *ptr and val
        : "+m"(*ptr), "+r"(val) 
        :
        : "memory"
    );
    return val;
}

void lock()
{
    while (test_and_set(&key, 1));
}

void unlock()
{
    key = 0;
}

void *thread_func(void *arg)
{
    int tid = *(int *)arg;

    lock();
    // printf("Thread %d locked\n", tid);
    for (int i = 0; i < NUM_ITERS; i++)
        shared_resource++;
    printf("Thread %d unlocked\n", tid);
    unlock();
    pthread_exit(NULL);
}

int main()
{
    int n = NUM_THREADS;
    pthread_t threads[n];
    int tids[n];

    for (int i = 0; i < n; i++)
    {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }

    for (int i = 0; i < n; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("expected: %d\n", NUM_THREADS * NUM_ITERS);
    printf("result: %d\n", shared_resource);

    return 0;
}