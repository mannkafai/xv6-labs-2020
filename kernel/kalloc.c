// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

#define PHYSTART KERNBASE
#define PAPN(pa) (((uint64)(pa)-PHYSTART) / PGSIZE)
int papage_refs[(PHYSTOP - PHYSTART) / PGSIZE];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

int
papage_incref(uint64 pa)
{
  int ref = 0;
  acquire(&kmem.lock);
  int pn = PAPN(pa);
  if (pa >= PHYSTOP || pa < PHYSTART || papage_refs[pn] < 1)
    panic("papage incref");
  papage_refs[pn]++;
  ref = papage_refs[pn];
  release(&kmem.lock);
  return ref;
}

int
papage_decref(uint64 pa)
{
  int ref = 0;
  acquire(&kmem.lock);
  int pn = PAPN(pa);
  if (papage_refs[pn] < 1)
    panic("papage decref");
  papage_refs[pn]--;
  ref = papage_refs[pn];
  release(&kmem.lock);
  return ref;
}

int
papage_refnum(uint64 pa)
{
  int ref = 0;
  acquire(&kmem.lock);
  ref = papage_refs[PAPN(pa)];
  release(&kmem.lock);
  return ref;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    papage_refs[PAPN(p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if (papage_decref((uint64)pa) > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
    kmem.freelist = r->next;
    int pn = PAPN(r);
    if (papage_refs[pn] != 0)
      panic("kalloc ref");
    papage_refs[pn] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
