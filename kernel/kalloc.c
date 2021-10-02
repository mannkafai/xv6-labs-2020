// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(int cpu, void *pa_start, void *pa_end);
void kfreecpu(int cpu, void *pa);
void *kalloccpu(int cpu);
int cpuid_off();

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  uint64 memsz_cpu = (PHYSTOP - (uint64)end) / NCPU;
  for (int i = 0; i < NCPU; i++)
  {
    initlock(&kmem[i].lock, "kmem");
    freerange(i, end + i * memsz_cpu, end + (i + 1) * memsz_cpu);
  }
}

void
freerange(int cpu, void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfreecpu(cpu, p);
}

void 
kfreecpu(int cpu, void* pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
}

void *
kalloccpu(int cpu)
{
  struct run *r;
  
  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if(r)
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


int 
cpuid_off()
{
  int id = 0;
  push_off();
  id = cpuid();
  pop_off();
  return id;
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  kfreecpu(cpuid_off(), pa);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  int cpu = cpuid_off();
  void *r = kalloccpu(cpu);
  if (r == 0)
  {
    for (int i = 0; i < NCPU; i++)
    {
      if (i == cpu)
        continue;
      r = kalloccpu(i);
      if (r)
        break;
    }
  }
  return r;
}
