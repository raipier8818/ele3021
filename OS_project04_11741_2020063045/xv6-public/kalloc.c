// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

// EDITED : Include proc.h to access the proc structure
#include "proc.h"


// EDITED : Function externs
extern int countfp(void);
extern int countvp(void);
extern int countpp(void);
extern int countptp(void);
extern pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);



void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

// EDITED
struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  int fpcnt;
  int pgrefcnt[PHYSTOP/PGSIZE];
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE){
    // EDITED : initialize the reference count of each page to 0
    kmem.pgrefcnt[V2P(p)/PGSIZE] = 0;
    kfree(p);
  }
}

//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

// EDITED
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  if (kmem.use_lock)
    acquire(&kmem.lock);

  // EDITED : Decrease reference count of the page
  if (kmem.pgrefcnt[V2P(v) / PGSIZE] > 0)
  {
    kmem.pgrefcnt[V2P(v) / PGSIZE]--;
  }

  // EDITED : If reference count is 0, free the page
  if (kmem.pgrefcnt[V2P(v) / PGSIZE] == 0)
  {
    // Fill with junk to catch dangling refs.
    memset(v, 1, PGSIZE);
    kmem.fpcnt++;
    
    r = (struct run*)v;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }

  if (kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  
  // EDITED : Decrease free page count
  kmem.fpcnt--;
  
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;

    // EDITED : Set reference count of the page to 1
    kmem.pgrefcnt[V2P((char*)r)/PGSIZE] = 1;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

void 
incr_refc(uint pa){
  if(kmem.use_lock)
    acquire(&kmem.lock);

  kmem.pgrefcnt[pa / PGSIZE]++;

  if(kmem.use_lock)
    release(&kmem.lock);
}

void 
decr_refc(uint pa)
{
  if(kmem.use_lock)
    acquire(&kmem.lock);

  kmem.pgrefcnt[pa / PGSIZE]--;

  if(kmem.use_lock)
    release(&kmem.lock);
}

int 
get_refc(uint pa)
{
  if(kmem.use_lock)
    acquire(&kmem.lock);

  int temp = kmem.pgrefcnt[pa / PGSIZE];

  if(kmem.use_lock)
    release(&kmem.lock);
  return temp;
}

int countfp(void) {
  if (kmem.use_lock)
    acquire(&kmem.lock);
  int temp = kmem.fpcnt;
  if (kmem.use_lock)
    release(&kmem.lock);
  return temp;
}

int countvp(void)
{
  struct proc* p = myproc();
  int sz = p->sz;

  int count = 0;
  
  for(int i = 0; i < sz; i += PGSIZE)
  {
    pte_t* pte = walkpgdir(p->pgdir, (void*)i, 0);
    if(pte && (*pte & PTE_U))
    {
      count++;
    }
  }

  return count;
}

int countpp(void)
{
  struct proc* p = myproc();
  pde_t* pgdir = p->pgdir;

  int count = 0;
  pde_t* pde;

  for (int i = 0; i < NPDENTRIES; i++)
  {
    pde = &pgdir[i];
    if (*pde & PTE_U)
    {
      pte_t* pte = (pte_t*)P2V(PTE_ADDR(*pde));
      for (int j = 0; j < NPTENTRIES; j++)
      {
        if (pte[j] & PTE_U)
        {
          count++;
        }
      }
    }
  }
  return count;
}

int countptp(void) {
  struct proc *curproc = myproc();
  pde_t* pgdir = curproc->pgdir;

  int count = 1;
  pde_t* pde;

  for (int i = 0; i < NPDENTRIES; i++)
  {
    pde = &pgdir[i];
    if (*pde & PTE_P)
    {
      count++;
    }
  }
  return count;
}

