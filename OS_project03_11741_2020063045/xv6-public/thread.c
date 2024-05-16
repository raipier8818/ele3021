#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "thread.h"

extern void forkret(void);
extern void trapret(void);
extern struct proc* allocproc(void);
extern void wakeup1(void *chan);
extern struct proc *initproc;

extern struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

typedef uint thread_t;

static uint nexttid = 1;

int 
_thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg){
    int i;
    struct proc *nt;
    struct proc *curproc = myproc();

    // Allocate thread structure
    if((nt = allocproc()) == 0){
        return -1;
    }
    acquire(&ptable.lock);

    // Set up new thread's properties
    nt->pid = curproc->pid;
    nt->tid = nexttid++;
    nt->main = curproc->main;
    nt->pgdir = curproc->pgdir;
    nt->parent = curproc->parent;
    *nt->tf = *curproc->tf;

    // Set up new thread's user memory
    nt->main->sz = PGROUNDUP(nt->main->sz);
    if((nt->main->sz = allocuvm(nt->main->pgdir, nt->main->sz, nt->main->sz + 2 * PGSIZE)) == 0){
        kfree(nt->kstack);
        nt->kstack = 0;
        nt->state = UNUSED;
        return -1;
    }
    clearpteu(nt->main->pgdir, (char*)(nt->main->sz - 2 * PGSIZE));
    nt->sz = nt->main->sz;

    // insert arguments into user stack
    uint ustack[2];
    uint sp = nt->sz;

    ustack[0] = 0xffffffff;
    ustack[1] = (uint)arg;
    sp -= 2 * sizeof(uint);

    if(copyout(nt->pgdir, sp, ustack, 2 * sizeof(uint)) < 0){
        kfree(nt->kstack);
        nt->kstack = 0;
        nt->state = UNUSED;
        return -1;
    }

    // Set up new thread's trapframe (sp, pc)
    nt->tf->eip = (uint)start_routine;
    nt->tf->esp = sp;

    // update size of proc
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if (p->pid == nt->pid)
        {
            p->sz = nt->sz;
        }
    }
    // Copy file descriptors
    for(i = 0; i < NOFILE; i++)
        if(curproc->ofile[i])
            nt->ofile[i] = filedup(curproc->ofile[i]);
    nt->cwd = idup(curproc->cwd);

    // set up new thread's name
    safestrcpy(nt->name, curproc->name, sizeof(curproc->name));


    *thread = nt->tid;
    nt->state = RUNNABLE;

    release(&ptable.lock);

    return 0;
}


void
_thread_exit(void *retval){
    struct proc *curthrd = myproc();
    struct proc *main = curthrd->main;
    struct proc *p;
    int fd;

    // if main thread, kill all threads and close all files
    if(curthrd == main){
        // kill all threads
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->pid == curthrd->pid && p->tid != curthrd->tid){
                p->killed = 1;
                if(p->state == SLEEPING)
                    p->state = RUNNABLE;
            }
        }

        // close all files
        for(fd = 0; fd < NOFILE; fd++){
            if(curthrd->ofile[fd]){
                fileclose(curthrd->ofile[fd]);
                curthrd->ofile[fd] = 0;
            }
        }
    }

    begin_op();
    iput(curthrd->cwd);
    end_op();
    curthrd->cwd = 0;

    acquire(&ptable.lock);

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if (p->parent->pid == curthrd->pid)
        {
            p->parent = initproc;
            if (p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }

    wakeup1(curthrd->main);

    // Jump into the scheduler, never to return.
    curthrd->state = ZOMBIE;
    curthrd->retval = retval;

    sched();
    panic("zombie exit");
}

int
_thread_join(thread_t thread, void** retval){
    struct proc *curthrd = myproc();
    struct proc *p;
    int havekids;

    acquire(&ptable.lock);
    for(;;){
        // Scan through table looking for exited children.
        havekids = 0;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->pid != curthrd->pid || p->tid != thread)
                continue;
            havekids = 1;
            if(p->state == ZOMBIE){
                // Found one.
                kfree(p->kstack);
                p->kstack = 0;
                p->state = UNUSED;
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->tid = 0;
                p->main = 0;

                *retval = p->retval;
                p->retval = 0;
                release(&ptable.lock);
                return 0;
            }
        }

        // No point waiting if we don't have any children.
        if(!havekids || curthrd->killed){
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curthrd, &ptable.lock);  //DOC: wait-sleep
    }
}
