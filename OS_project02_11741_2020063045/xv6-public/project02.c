

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main_pid;

void countqlevel(int result[5], int tick){
    int start = uptime();
    while(uptime() - start < tick){
        int lev = getlev();
        if(lev < 0 || lev > 4){
            if(lev != 99){
                return;
            }
        }
        if(lev == 99){
            result[4]++;
        } else {
            result[lev]++;
        }
    }
}

void test(const char* testname, int (*testfunc)(void)) {
    printf(1, "-------------\n");
    printf(1, "Running %s\n", testname);
    if (testfunc() == 0) {
        printf(1, "Test %s passed\n", testname);
    } else {
        printf(1, "Test %s failed\n", testname);
    }
    printf(1, "-------------\n");
}

void print_test_results(int results[5]) {
    printf(1, "[ Process %d ]\n", getpid());
    for(int i = 0; i < 4; i++){
        printf(1, "\tL%d: %d\n", i, results[i]);
    }
    printf(1, "\tMoQ: %d\n", results[4]);
}

int mlfq_l0_test(void){
    // set test parameters
    int p1 = fork();
    if(p1 == 0){
        int result[5] = {0, 0, 0, 0, 0};

        countqlevel(result, 1);
        // compare result and expected
        for(int i = 1; i < 5; i++){
            if(result[i] != 0){
                printf(1, "Test failed\n");
                print_test_results(result);
                exit();
            }
        }
        // print result
        print_test_results(result);
        exit();
    } else {
        wait();
    }
    return 0;
}

int preemption_test(void)
{
    int pid;
    int start_time, end_time;

    start_time = uptime(); // 현재 시간 측정

    pid = fork();
    if (pid < 0)
    {
        printf(1, "fork failed\n");
        return -1;
    }
    else
    {
        printf(1, "pid : %d, qlevel : %d\n", getpid(), getlev());
    }

    if (pid == 0)
    { // child process
        yield();
        exit();
    }

    wait(); // child process가 종료될 때까지 기다림

    end_time = uptime(); // 현재 시간 측정

    printf(1, "preemption time: %d\n", end_time - start_time);

    return 0;
}

int trap_test(){
    if (fork() == 0)
    {
        int *ptr = 0;
        printf(1, "Read from null: %x\n", *ptr);
        *ptr = 0;
        printf(1, "Write to null\n");
        exit();
    }
    wait();

    if (fork() == 0)
    {
        // divide by zero test
        int a = 5;
        int b = 0;
        int result = a / b;
        printf(1, "%d\n", result);
        exit();
    }
    wait();

    if (fork() == 0)
    {
        // illegal instruction test
        asm volatile("ud2");
        exit();
    }
    wait();

    return 0;
}

int main(int argc, char *argv[])
{
    main_pid = getpid();
    test("MLFQ L0 Test", mlfq_l0_test);
    test("Preemption Test", preemption_test);
    test("Trap Test", trap_test);
    exit();
}
