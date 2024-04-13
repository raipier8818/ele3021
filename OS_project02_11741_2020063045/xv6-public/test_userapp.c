#include "types.h"
#include "stat.h"
#include "user.h"

#define RN_STUDENT_ID 2020063045

void level_cnt_init(int *level_cnt)
{
    level_cnt[0] = level_cnt[1] = level_cnt[2] = level_cnt[3] = level_cnt[4] = 0;
}

void add_level_cnt(int *level_cnt)
{
    int lev;

    lev = getlev();
    if (lev == 99)
        ++level_cnt[4];
    else
        ++level_cnt[lev];
}

void print_level_cnt(int *level_cnt, const char *pname)
{
    printf(1, " [ process %d (%s) ]\n", getpid(), pname);
    printf(1, "\t- L0:\t\t%d\n", level_cnt[0]);
    printf(1, "\t- L1:\t\t%d\n", level_cnt[1]);
    printf(1, "\t- L2:\t\t%d\n", level_cnt[2]);
    printf(1, "\t- L3:\t\t%d\n", level_cnt[3]);
    printf(1, "\t- MONOPOLIZED:\t%d\n\n", level_cnt[4]);
}

void rn_test1()
{
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test1 [L0 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0)
    {
        for (int cnt = 1000; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0)
    {
        for (int cnt = 500; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0)
    {
        for (int cnt = 100; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    w_list[0] = p3;
    w_list[1] = p2;
    w_list[2] = p1;
    w_cnt = 0;
    for (; w_cnt < 3;)
    {
        p = wait();
        if (p == w_list[w_cnt])
            ++w_cnt;
        else
        {
            printf(1, "rn_test1 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test1 finished successfully\n\n\n");
}

void rn_test2()
{
    int p, p1, p2, p3, p4, w_cnt, w_list[4], level_cnt[5];

    printf(1, "rn_test2 [L1~L2 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0)
    {
        for (int cnt = 300; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0)
    {
        for (int cnt = 200; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0)
    {
        for (int cnt = 100; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0)
    {
        for (int cnt = 50; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    if (p4 % 2 == 1)
    {
        w_list[0] = p4;
        w_list[1] = p2;
        w_list[2] = p3;
        w_list[3] = p1;
    }
    else
    {
        w_list[0] = p3;
        w_list[1] = p1;
        w_list[2] = p4;
        w_list[3] = p2;
    }
    w_cnt = 0;
    for (; w_cnt < 4;)
    {
        p = wait();
        if (p == w_list[w_cnt])
            ++w_cnt;
        else
        {
            printf(1, "rn_test2 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test2 finished successfully\n\n\n");
}

void rn_test3()
{
    int p, p1, p2, p3, p4, w_cnt, w_list[4], level_cnt[5];

    printf(1, "rn_test3 [L3 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 1) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 7) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    w_list[0] = p2;
    w_list[1] = p4;
    w_list[2] = p3;
    w_list[3] = p1;
    w_cnt = 0;
    for (; w_cnt < 4;)
    {
        p = wait();
        if (p == w_list[w_cnt])
            ++w_cnt;
        else
        {
            printf(1, "rn_test3 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test3 finished successfully\n\n\n");
}

void rn_test4()
{
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test4 [MoQ Only - wait 5sec]\n");

    level_cnt_init(level_cnt);

    p1 = fork();
    if (p1 == 0)
    {
        if (setmonopoly(getpid(), RN_STUDENT_ID) < 0)
            printf(1, "setmonopoly failed\n");
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 100; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0)
    {
        if (setmonopoly(getpid(), RN_STUDENT_ID) < 0)
            printf(1, "setmonopoly failed\n");
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0)
    {
        if (setmonopoly(getpid(), RN_STUDENT_ID) < 0)
            printf(1, "setmonopoly failed\n");
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    rn_sleep(5000);
    printf(1, "monopolize!\n");
    monopolize();

    w_list[0] = p1;
    w_list[1] = p2;
    w_list[2] = p3;
    w_cnt = 0;
    for (; w_cnt < 3;)
    {
        p = wait();
        if (p == w_list[w_cnt])
            ++w_cnt;
        else
        {
            printf(1, "rn_test4 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test4 finished successfully\n\n\n");
}

void rn_test5()
{
    int p, p1, p2, p3, p4, p5, p6, w_cnt, w_list[6], level_cnt[5];

    printf(1, "rn_test5 [L0, L1, L2, L3]\n");

    level_cnt_init(level_cnt);
    p1 = fork();

    // 3600 ticks
    if (p1 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();

    // 3600 ticks
    if (p2 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();

    // 4000 ticks
    if (p3 == 0)
    {
        for (int cnt = 200; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();

    // 4000 ticks
    if (p4 == 0)
    {
        for (int cnt = 200; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    p5 = fork();

    // 3000 ticks
    if (p5 == 0)
    {
        for (int cnt = 1000; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p5");
        exit();
    }

    p6 = fork();

    // 1500 ticks
    if (p6 == 0)
    {
        for (int cnt = 500; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p6");
        exit();
    }

    w_list[0] = p6;
    w_list[1] = p5;
    if (p3 % 2 == 1)
    {
        w_list[2] = p3;
        w_list[3] = p4;
    }
    else
    {
        w_list[2] = p4;
        w_list[3] = p3;
    }
    w_list[4] = p1;
    w_list[5] = p2;
    w_cnt = 0;
    for (; w_cnt < 6;)
    {
        p = wait();
        if (p == w_list[w_cnt])
            ++w_cnt;
        else
        {
            printf(1, "rn_test5 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test5 finished successfully\n\n\n");
}

void rn_test6()
{
    int p, p1, p2, p3, p4, p5, p6, p7, w_cnt, w_list[7], level_cnt[5];

    printf(1, "rn_test6 [L0, L1, L2, L3, MoQ]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0)
    {
        for (int cnt = 60; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0)
                printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0)
    {
        for (int cnt = 200; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0)
    {
        for (int cnt = 200; cnt--;)
        {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    p5 = fork();
    if (p5 == 0)
    {
        for (int cnt = 1000; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p5");
        exit();
    }

    p6 = fork();
    if (p6 == 0)
    {
        for (int cnt = 500; cnt--;)
        {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p6");
        exit();
    }

    p7 = fork();
    if (p7 == 0)
    {
        if (setmonopoly(getpid(), RN_STUDENT_ID) < 0)
            printf(1, "setmonopoly failed\n");
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--;)
        {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p7");
        exit();
    }

    w_list[0] = p6;
    w_list[1] = p5;
    if (p3 % 2 == 1)
    {
        w_list[2] = p3;
        w_list[4] = p4;
    }
    else
    {
        w_list[2] = p4;
        w_list[4] = p3;
    }
    w_list[3] = p7;
    w_list[5] = p1;
    w_list[6] = p2;
    w_cnt = 0;
    for (; w_cnt < 7;)
    {
        p = wait();
        if (p == w_list[w_cnt])
            ++w_cnt;
        else
        {
            printf(1, "rn_test6 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
        if (w_cnt == 3)
        {
            printf(1, "monopolize!\n");
            monopolize();
        }
    }

    printf(1, "rn_test6 finished successfully\n\n\n");
}

int main(int argc, char **argv)
{
    rn_test1();
    rn_test2();
    rn_test3();
    rn_test4();
    // monopolize();
    rn_test5();
    rn_test6();

    exit();
}

// #include "types.h"
// #include "stat.h"
// #include "user.h"

// #define NUM_LOOP 100000
// #define NUM_SLEEP 500

// #define NUM_THREAD 8
// #define MAX_LEVEL 5
// int parent;

// int fork_children()
// {
//     int i, p;
//     for (i = 0; i < NUM_THREAD; i++)
//         if ((p = fork()) == 0)
//         {
//             sleep(10);
//             return getpid();
//         }
//     return parent;
// }

// int fork_children2()
// {
//     int i, p;
//     for (i = 0; i < NUM_THREAD; i++)
//     {
//         if ((p = fork()) == 0)
//         {
//             sleep(10);
//             return getpid();
//         }
//         else
//         {
//             int r = setpriority(p, i + 1);
//             if (r < 0)
//             {
//                 printf(1, "setpriority returned %d\n", r);
//                 exit();
//             }
//         }
//     }
//     return parent;
// }

// int fork_children3()
// {
//     int i, p;
//     for (i = 0; i <= NUM_THREAD; i++)
//     {
//         if ((p = fork()) == 0)
//         {
//             sleep(10);
//             return getpid();
//         }
//         else
//         {
//             int r = 0;
//             if (p % 2 == 1)
//             {
//                 r = setmonopoly(p, 2020063045); // input your student number
//                 printf(1, "Number of processes in MoQ: %d\n", r);
//             }
//             if (r < 0)
//             {
//                 printf(1, "setmonopoly returned %d\n", r);
//                 exit();
//             }
//         }
//     }
//     return parent;
// }
// void exit_children()
// {
//     if (getpid() != parent)
//         exit();
//     while (wait() != -1)
//         ;
// }

// int main(int argc, char *argv[])
// {
//     int i, pid;
//     int count[MAX_LEVEL] = {0};

//     parent = getpid();

//     printf(1, "MLFQ test start\n");

//     printf(1, "[Test 1] default\n");
//     pid = fork_children();

//     if (pid != parent)
//     {
//         for (i = 0; i < NUM_LOOP; i++)
//         {
//             int x = getlev();
//             if (x < 0 || x > 3)
//             {
//                 if (x != 99)
//                 {
//                     printf(1, "Wrong level: %d\n", x);
//                     exit();
//                 }
//             }
//             if (x == 99)
//                 count[4]++;
//             else
//                 count[x]++;
//         }
//         printf(1, "Process %d\n", pid);
//         for (i = 0; i < MAX_LEVEL - 1; i++)
//             printf(1, "L%d: %d\n", i, count[i]);
//         printf(1, "MoQ: %d\n", count[4]);
//     }
//     exit_children();
//     printf(1, "[Test 1] finished\n");

//     printf(1, "[Test 2] priorities\n");
//     pid = fork_children2();

//     if (pid != parent)
//     {
//         for (i = 0; i < NUM_LOOP; i++)
//         {
//             int x = getlev();
//             if (x < 0 || x > 3)
//             {
//                 if (x != 99)
//                 {
//                     printf(1, "Wrong level: %d\n", x);
//                     exit();
//                 }
//             }
//             if (x == 99)
//                 count[4]++;
//             else
//                 count[x]++;
//         }
//         printf(1, "Process %d\n", pid);
//         for (i = 0; i < MAX_LEVEL - 1; i++)
//             printf(1, "L%d: %d\n", i, count[i]);
//         printf(1, "MoQ: %d\n", count[4]);
//     }
//     exit_children();
//     printf(1, "[Test 2] finished\n");

//     printf(1, "[Test 3] sleep\n");
//     pid = fork_children2();

//     if (pid != parent)
//     {
//         for (i = 0; i < NUM_SLEEP; i++)
//         {
//             int x = getlev();
//             if (x < 0 || x > 3)
//             {
//                 if (x != 99)
//                 {
//                     printf(1, "Wrong level: %d\n", x);
//                     exit();
//                 }
//             }
//             if (x == 99)
//                 count[4]++;
//             else
//                 count[x]++;
//             sleep(1);
//         }
//         printf(1, "Process %d\n", pid);
//         for (i = 0; i < MAX_LEVEL - 1; i++)
//             printf(1, "L%d: %d\n", i, count[i]);
//         printf(1, "MoQ: %d\n", count[4]);
//     }
//     exit_children();
//     printf(1, "[Test 3] finished\n");

//     printf(1, "[Test 4] MoQ\n");
//     pid = fork_children3();

//     if (pid != parent)
//     {
//         if (pid == 36)
//         {
//             monopolize();
//             exit();
//         }
//         for (i = 0; i < NUM_LOOP; i++)
//         {
//             int x = getlev();
//             if (x < 0 || x > 3)
//             {
//                 if (x != 99)
//                 {
//                     printf(1, "Wrong level: %d\n", x);
//                     exit();
//                 }
//             }
//             if (x == 99)
//                 count[4]++;
//             else
//                 count[x]++;
//         }
//         printf(1, "Process %d\n", pid);
//         for (i = 0; i < MAX_LEVEL - 1; i++)
//             printf(1, "L%d: %d\n", i, count[i]);
//         printf(1, "MoQ: %d\n", count[i]);
//     }
//     exit_children();
//     printf(1, "[Test 4] finished\n");

//     exit();
// }
