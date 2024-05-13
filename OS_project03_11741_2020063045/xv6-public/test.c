#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREAD 5
#define MAX_NUM_THREAD 62

thread_t thread[MAX_NUM_THREAD];

int status;
int *ptr;

// common functions

void rn_test_fail()
{
    printf(1, "Test Failed\n");
    exit();
}

void create_all(int offset, int n, void *(*entry)(void *))
{
    int i;

    for (i = 0; i < n; ++i)
    {
        if (thread_create(&thread[offset + i], entry, (void *)(offset + i)) != 0)
        {
            printf(1, "Error: creating thread %d\n", offset + i);
            rn_test_fail();
        }
    }
}

void join_all(int offset, int n)
{
    int i, retval;

    for (i = 0; i < n; ++i)
    {
        if (thread_join(thread[offset + i], (void **)&retval) != 0)
        {
            printf(1, "Error joining thread %d\n", offset + i);
            rn_test_fail();
        }
        if (retval != offset + i)
        {
            printf(1, "Thread %d returned %d, but expected %d\n", offset + i, retval, offset + i);
            rn_test_fail();
        }
    }
}

void *rn_test1_thread(void *arg)
{
    int val = (int)arg;
    printf(1, "T %d S\n", val);
    if (val == 1)
    {
        sleep(200);
        status = 1;
    }
    printf(1, "T %d E\n", val);
    thread_exit(arg);
    return 0;
}
void rn_test1()
{
    printf(1, "rn_test1 [Basic]\n");

    status = 2;
    create_all(0, 2, rn_test1_thread);
    sleep(100);
    join_all(0, 2);

    if (status != 1)
    {
        printf(1, "Join returned before thread exit, or the address space is not properly shared\n");
        rn_test_fail();
    }

    printf(1, "rn_test1 Passed\n\n");
    exit();
}

void *rn_test2_thread(void *arg)
{
    int val = (int)arg;
    int pid;

    printf(1, "T %d S\n", val);
    pid = fork();
    if (pid < 0)
    {
        printf(1, "Fork error on thread %d\n", val);
        rn_test_fail();
    }

    if (pid == 0)
    {
        printf(1, "C %d S\n", val);
        sleep(100);
        status = 3;
        printf(1, "C %d E\n", val);
        exit();
    }
    else
    {
        status = 2;
        if (wait() == -1)
        {
            printf(1, "Thread %d lost their child\n", val);
            rn_test_fail();
        }
    }
    printf(1, "T %d E\n", val);
    thread_exit(arg);
    return 0;
}
void rn_test2()
{
    printf(1, "rn_test2 [Fork]\n");

    status = 0;

    create_all(0, NUM_THREAD, rn_test2_thread);
    join_all(0, NUM_THREAD);
    if (status != 2)
    {
        if (status == 3)
            printf(1, "Child process referenced parent's memory\n");
        else
            printf(1, "Status expected 2, found %d\n", status);
        rn_test_fail();
    }

    printf(1, "rn_test2 Passed\n\n");
    exit();
}

void *rn_test3_thread(void *arg)
{
    int val = (int)arg;
    int i, j;
    int *p;

    printf(1, "T %d S\n", val);

    if (val == 0)
    {
        ptr = (int *)malloc(65536);
        sleep(100);
        free(ptr);
        ptr = 0;
    }
    else
    {
        for (; ptr == 0;)
            sleep(1);
        for (i = 0; i < 16384; ++i)
            ptr[i] = val;
    }

    for (; ptr != 0;)
        sleep(1);

    for (i = 0; i < 2000; ++i)
    {
        p = (int *)malloc(65536);
        for (j = 0; j < 16384; ++j)
            p[j] = val;
        for (j = 0; j < 16384; ++j)
        {
            if (p[j] != val)
            {
                printf(1, "Thread %d found %d\n", val, p[j]);
                rn_test_fail();
            }
        }
        free(p);
    }

    thread_exit(arg);
    return 0;
}
void rn_test3()
{
    printf(1, "rn_test3 [Sbrk]\n");

    create_all(0, NUM_THREAD, rn_test3_thread);
    join_all(0, NUM_THREAD);

    printf(1, "rn_test3 Passed\n\n");
    exit();
}

void *rn_test4_thread1(void *arg)
{
    sleep(200);
    rn_test_fail();
    exit();
    return 0;
}
void *rn_test4_thread2(void *arg)
{
    int val = (int)arg;
    sleep(100);
    printf(1, "T %d S\n", val);
    if (val > 10000)
    {
        printf(1, "Kill %d\n", val - 10000);
        kill(val - 10000);
        status = 1;
    }
    else
    {
        for (; status != val;)
            sleep(1);
        ++status;
    }
    // printf(1, "This code should be executed %d times.\n", NUM_THREAD);
    // printf(1, "TCE\n");
    thread_exit(arg);
    return 0;
}
void rn_test4()
{
    int pid, retval;

    printf(1, "rn_test4 [Thread Kill]\n");

    pid = fork();
    if (pid < 0)
    {
        printf(1, "Fork failed!!\n");
        rn_test_fail();
    }
    else if (pid == 0)
    {
        create_all(NUM_THREAD, NUM_THREAD, rn_test4_thread1);
        sleep(300);
        rn_test_fail();
    }
    else
    {
        status = 0;

        if (thread_create(&thread[0], rn_test4_thread2, (void *)(pid + 10000)) != 0)
        {
            printf(1, "Error: creating thread %d\n", 0);
            rn_test_fail();
        }
        create_all(1, NUM_THREAD - 1, rn_test4_thread2);
        thread_join(thread[0], (void **)&retval);
        join_all(1, NUM_THREAD - 1);

        for (; wait() != -1;)
            ;

        if (status != NUM_THREAD)
        {
            printf(1, "Thread2 executed %d times, expected %d\n", status, NUM_THREAD);
            rn_test_fail();
        }

        printf(1, "rn_test4 Passed\n\n");
        exit();
    }
}

void *rn_test5_thread(void *arg)
{
    int val = (int)arg;
    printf(1, "T %d S\n", val);
    if (arg == 0)
    {
        sleep(100);
        printf(1, "Exit\n");
        exit();
    }
    else
        sleep(200);

    rn_test_fail();
    return 0;
}
void rn_test5()
{
    int pid;

    printf(1, "rn_test5 [Thread Exit]\n");

    if ((pid = fork()) == 0)
    {
        create_all(0, NUM_THREAD, rn_test5_thread);
        sleep(200);
        for (;;)
            ;
    }
    else
    {
        for (; wait() != pid;)
            ;
    }

    printf(1, "rn_test5 Passed\n\n");
    exit();
}

void *rn_test6_thread(void *arg)
{
    int val = (int)arg;
    printf(1, "T %d S\n", val);
    if (arg == 0)
    {
        sleep(100);
        char *pname = "/echo";
        char *args[3] = {pname, "'exec echo'", 0};
        printf(1, "Exec\n");
        exec(pname, args);
    }
    else
    {
        sleep(200);
    }

    rn_test_fail();
    return 0;
}
void rn_test6()
{
    int pid;

    printf(1, "rn_test6 [Thread Exec]\n");

    if ((pid = fork()) == 0)
    {
        create_all(0, NUM_THREAD, rn_test6_thread);
        sleep(200);
        for (;;)
            ;
    }
    else
    {
        for (; wait() != pid;)
            ;
    }

    printf(1, "rn_test6 Passed\n\n");
    exit();
}
int main(int argc, char *argv[])
{
    int i, pid;
    void (*rn_test[])() = {rn_test1, rn_test2, rn_test3, rn_test4, rn_test5, rn_test6};

    for (i = 0; i < sizeof rn_test / sizeof *rn_test; ++i)
    {
        if ((pid = fork()) == 0)
            rn_test[i]();
        else
            for (; wait() != pid;)
                ;
    }

    printf(1, "rn_test finish\n");

    exit();
}
