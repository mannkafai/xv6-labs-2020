
#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "vma.h"

struct {
  struct spinlock lock;
  struct vma vmas[NFILE];
} vmatable;

void
vma_init(void)
{
  initlock(&vmatable.lock, "vmatable");
}

// Allocate a file structure.
struct vma*
vma_alloc(void)
{
  struct vma *v;

  acquire(&vmatable.lock);
  for(v = vmatable.vmas; v < vmatable.vmas + NFILE; v++){
    if(v->file == 0){
      release(&vmatable.lock);
      return v;
    }
  }
  release(&vmatable.lock);
  return 0;
}

void 
vma_free(struct vma *v)
{
    acquire(&vmatable.lock);
    v->file = 0;
    release(&vmatable.lock);
}