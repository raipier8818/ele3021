#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "queue.h"

// EDITED
enum _sched_mode sched_mode;
struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
  struct queue mlfq[4];
  struct queue moq;
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

// EDITED : debug print functions
void printproc(struct proc *p)
{
  cprintf("{\n \"pid\": %d,\n  \"name\": \"%s\",\n  \"state\": %d,\n  \"priority\": %d,\n  \"qlevel\": %d,\n  \"ticks\": %d\n}\n", p->pid, p->name, p->state, p->priority, p->qlevel, p->ticks);
}

void printqueue(struct queue *q)
{
  cprintf("{\n \"size\": %d,\n  \"head\": %d,\n  \"tail\": %d,\n  \"idxs\": [", q->size, q->head, q->tail);
  for (int i = q->head; i != q->tail; i = (i + 1) % (NPROC + 1))
  {
    cprintf("%d, ", q->idxs[i]);
  }
  cprintf("]\n}\n");
}

void pinit(void)
{
  initlock(&ptable.lock, "ptable");

  // EDITED : initialize queues
  for (int i = 0; i < 3; i++)
  {
    init(&ptable.mlfq[i]);
  }
  init(&ptable.moq);

  // EDITED : initialize sched_mode
  sched_mode = MLFQ;
}

// Must be called with interrupts disabled
int cpuid()
{
  return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i)
  {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// PAGEBREAK: 32
//  Look in the process table for an UNUSED proc.
//  If found, change state to EMBRYO and initialize
//  state required to run in the kernel.
//  Otherwise return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  // EDITED : initialize process and push to L0 queue
  p->priority = 0;
  p->qlevel = 0;
  p->idx = p - ptable.proc;
  push(&ptable.mlfq[0], p->idx);

  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0)
  {
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// PAGEBREAK: 32
//  Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if (n > 0)
  {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  else if (n < 0)
  {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++)
  {
    if (curproc->ofile[fd])
    {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curproc)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.

  curproc->state = ZOMBIE;

  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}


// EDITED

// Delete process index from queue q if state is ZOMBIE or UNUSED
void optimqueue(){
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if((p->state == ZOMBIE || p->state == UNUSED) && p->pid != 0){
      if(p->qlevel == 99 && sched_mode == MoQ){
        delete(&ptable.moq, p->idx);
      }
      else{
        delete(&ptable.mlfq[p->qlevel], p->idx);
      }
    }
  }
}

// Find next process index to run
int findnextprocidx()
{
  if(sched_mode == MoQ){
    struct queue *q = &ptable.moq;
    struct proc *p;
    for (int idx = q->head; idx != q->tail; idx = (idx + 1) % (NPROC + 1))
    {
      p = &ptable.proc[q->idxs[idx]];
      if (p->state == RUNNABLE || p->state == SLEEPING)
      {
        return q->idxs[idx];
      }
    }
    return -1;
  }

  // 0 ~ 2 : round robin scheduling
  for (int i = 0; i < 3; i++)
  {
    struct queue *q = &ptable.mlfq[i];
    for (int idx = q->head; idx != q->tail; idx = (idx + 1) % (NPROC + 1))
    {
      struct proc *p = &ptable.proc[q->idxs[idx]];
      if (p->state == RUNNABLE)
      {
        return q->idxs[idx];
      }
    }
  }

  struct queue *q = &ptable.mlfq[3];
  struct proc *p, *next_p;
  int max_priority = -1;

  // 3 : priority scheduling
  for (int idx = q->head; idx != q->tail; idx = (idx + 1) % (NPROC + 1))
  {
    p = &ptable.proc[q->idxs[idx]];
    if (p->state == RUNNABLE && max_priority < p->priority)
    {
      max_priority = p->priority;
      next_p = p;
    }

  }

  if (max_priority != -1)
  {
    return next_p->idx;
  }
  return -1;
}

// Set process priority
int _setpriority(int pid, int priority)
{
  if (priority < 0 || priority > 10)
  {
    return -2;
  }

  acquire(&ptable.lock);  
  int i = 0;
  for(; i < NPROC; i++){
    if(ptable.proc[i].pid == pid){
      break;
    }
  }
  
  if (i == NPROC)
  {
    release(&ptable.lock);
    return -1;
  }
  struct proc *p = &ptable.proc[i];
  
  p->priority = priority;
  release(&ptable.lock);
  return 0;
}

// insert proc into moq
int _setmonopoly(int pid, int password)
{

  if (password != 2020063045)
  {
    return -2;
  }

  acquire(&ptable.lock);
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      delete(&ptable.mlfq[p->qlevel], p->idx);
      p->qlevel = 99;
      push(&ptable.moq, p->idx);

      release(&ptable.lock);
      return ptable.moq.size;
    }
  }
  release(&ptable.lock);
  return -1;
}

// monopolize
void _monopolize(void)
{
  sched_mode = MoQ;
  yield();
}

// unmonopolize
void _unmonopolize(void)
{
  sched_mode = MLFQ;
  acquire(&tickslock);
  global_ticks = 0;
  release(&tickslock);
}

// set all proc's qlevel to 0 except monopolized proc
void priorityboost(void)
{
  if(sched_mode == MoQ)
  {
    return;
  }
  acquire(&ptable.lock);
  init(&ptable.mlfq[1]);
  init(&ptable.mlfq[2]);
  init(&ptable.mlfq[3]);

  for (int i = 0; i < NPROC; i++)
  {
    if (ptable.proc[i].state != UNUSED && ptable.proc[i].state != ZOMBIE && ptable.proc[i].qlevel != 0 && ptable.proc[i].qlevel != 99)
    {
      ptable.proc[i].qlevel = 0;
      ptable.proc[i].ticks = 0;
      push(&ptable.mlfq[0], i);
    }
  }
  release(&ptable.lock);
}

// move next proc to next queue
void movenext(struct proc *p)
{
  delete (&ptable.mlfq[p->qlevel], p->idx);
  if (p->qlevel == 0)
  {
    p->qlevel = p->pid % 2 == 0 ? 2 : 1;
  }
  else if (p->qlevel == 1 || p->qlevel == 2)
  {
    p->qlevel = 3;
  }
  else if (p->qlevel == 3)
  {
    p->priority = p->priority - 1 > 0 ? p->priority - 1 : 0;
  }
  push(&ptable.mlfq[p->qlevel], p->idx);
}

// PAGEBREAK: 42
//  Per-CPU process scheduler.
//  Each CPU calls scheduler() after setting itself up.
//  Scheduler never returns.  It loops, doing:
//   - choose a process to run
//   - swtch to start running that process
//   - eventually that process transfers control
//       via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;


  for (;;)
  {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    // EDITED
    acquire(&ptable.lock);
    optimqueue();

    // monopolized
    if (sched_mode == MoQ)
    {
      int nextpidx = findnextprocidx();
      if (nextpidx == -1)
      {
        _unmonopolize();
        release(&ptable.lock);
        continue;
      }
      p = &ptable.proc[nextpidx];
      if(p->state == SLEEPING){
        release(&ptable.lock);
        continue;
      }

      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&c->scheduler, p->context);
      switchkvm();
      c->proc = 0;
    }
    // not monopolized
    else
    {
      // find next proc idx
      int nextpidx = findnextprocidx();
      if (nextpidx == -1)
      {
        release(&ptable.lock);
        continue;
      }

      // find next proc in ptable proc
      p = &ptable.proc[nextpidx];
      p->ticks = 0;

      // switch to next proc
      c->proc = p;

      switchuvm(p);
      p->state = RUNNING;
      swtch(&c->scheduler, p->context);
      switchkvm();

      c->proc = 0;

      if(p->ticks >= p->qlevel * 2 + 2)
      {
        movenext(p);
      }
      else{
        delete(&ptable.mlfq[p->qlevel], p->idx);
        push(&ptable.mlfq[p->qlevel], p->idx);
      }
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&ptable.lock))
    panic("sched ptable.lock");
  if (mycpu()->ncli != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (readeflags() & FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{ 
  acquire(&ptable.lock); // DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first)
  {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  if (lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if (lk != &ptable.lock)
  {                        // DOC: sleeplock0
    acquire(&ptable.lock); // DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;


  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock)
  { // DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
    }
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// PAGEBREAK: 36
//  Print a process listing to console.  For debugging.
//  Runs when user types ^P on console.
//  No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [EMBRYO] "embryo",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if (p->state == SLEEPING)
    {
      getcallerpcs((uint *)p->context->ebp + 2, pc);
      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
