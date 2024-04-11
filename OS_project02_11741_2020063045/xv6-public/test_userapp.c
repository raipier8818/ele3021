#include "types.h"
#include "user.h"

int main(){
    int pid = fork();
    pid = fork();
    yield();
    if (pid == 0) {
        int cnt = 100;
        while (cnt--) {
            // Child process loop code here
            // sleep(100);
            printf(1, "pid : %d\n", getpid());
        }
    }else{
        wait();
        // wait(); // create zombie proc
    }

    exit();
}