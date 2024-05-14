#include <stdio.h>
#include <pthread.h>

int shared_resource = 0;

#define NUM_ITERS 100000
#define NUM_THREADS 100

void lock();
void unlock();

int flag = 0;
int turn[NUM_THREADS];

void lock(int tid)
{
    // __asm__ __volatile__ 
    // (
    //     "spin_lock: \n"
    //     "movl $1, %%eax \n"
    //     "xchg %%eax, %0 \n"
    //     "test %%eax, %%eax \n"
    //     "jnz spin_lock \n"
    //     : "+m" (flag)
    //     :
    //     : "%eax"
    // );
    flag = 1;
    while(flag == 1);
}

void unlock()
{  
    // __asm__ __volatile__
    // (
    //     "movl $0, %0 \n"
    //     : "+m"(flag)
    //     :
    //     :
    // );
    flag = 0;
}

void* thread_func(void* arg) {
    int tid = *(int*)arg;
    
    lock(tid);
    
        for(int i = 0; i < NUM_ITERS; i++)    shared_resource++;
    
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