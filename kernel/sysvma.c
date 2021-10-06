#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fcntl.h"
#include "sleeplock.h"
#include "vma.h"
#include "fs.h"
#include "file.h"

static int
proc_vma_fdalloc(struct proc *p)
{
    int i = 0;
    acquire(&p->lock);
    for (i = 0; i < NOFILE; i++){
        if (p->vma[i] == 0)
            break;
    }
    release(&p->lock);
    return i == NOFILE ? -1 : i;
}

static int
proc_vma_fd(struct proc *p, uint64 addr, int range)
{
    int i = 0;
    struct vma* vma = 0;
    acquire(&p->lock);
    for (i = 0; i < NOFILE; i++)
    {
        if (p->vma[i] == 0)
            continue;
        vma = p->vma[i];
        if ((range && addr >= vma->addr && addr < (vma->addr + vma->length)) ||
            (!range && addr == vma->addr))
            break;
    }
    release(&p->lock);
    return i == NOFILE ? -1 : i;
}

uint64
sys_mmap(void)
{
    uint64 addr;
    int length, prot, flags, fd, offset;
    if (argaddr(0, &addr) < 0 || argint(1, &length) < 0 ||
        argint(2, &prot) < 0 || argint(3, &flags) < 0 ||
        argint(4, &fd) < 0 || argint(5, &offset) < 0)
        return -1;

    struct proc *p = myproc();
    if (fd < 0 || fd >= NOFILE || p->ofile[fd] == 0)
        return -1;

    if (!p->ofile[fd]->readable && (prot & PROT_READ))
        return -1;

    if (!p->ofile[fd]->writable && (prot & PROT_WRITE) && flags == MAP_SHARED)
        return -1;

    int vfd = proc_vma_fdalloc(p);
    if (vfd < 0)
        return -1;

    struct vma *vma = vma_alloc();
    if (vma == 0)
        return -1;

    addr = p->sz;
    if (proc_lazygrow(length) < 0)
        return -1;

    p->vma[vfd] = vma;
    vma->addr = addr;
    vma->length = length;
    vma->prot = prot;
    vma->flags = flags;
    vma->file = filedup(p->ofile[fd]);

    return addr;
}

uint64
sys_munmap(void)
{
    uint64 addr;
    int length;
    if (argaddr(0, &addr) < 0 || argint(1, &length) < 0)
        return -1;

    struct proc *p = myproc();
    int fd = proc_vma_fd(p, addr, 0);
    if (fd < 0)
        return -1;
    struct vma *vma = p->vma[fd];
    if (vma == 0)
        return -1;

    if (length > vma->length)
        length = vma->length;

    if ((vma->prot & PROT_WRITE) && vma->flags == MAP_SHARED)
    {
        begin_op();
        ilock(vma->file->ip);
        writei(vma->file->ip, 1, vma->addr, 0, length);
        iunlock(vma->file->ip);
        end_op();
    }
    uvmunmap(p->pagetable, PGROUNDDOWN(vma->addr), length / PGSIZE, 1);
    if (length == vma->length)
    {
        fileclose(vma->file);
        vma_free(vma);
        p->vma[fd] = 0;
    }
    else
    {
        vma->addr += length;
        vma->length -= length;
    }
    return 0;
}

int 
proc_pagefault_mmap(pagetable_t pagetable, uint64 va, uint64* pa)
{
    struct proc *p = myproc();
    if (va >= p->sz)
        return -1;
    if (va < PGROUNDDOWN(p->trapframe->sp))
        return -1;

    int fd = proc_vma_fd(p, va, 1);
    if (fd < 0)
        return -1;
    struct vma *vma = p->vma[fd];
    if (vma == 0)
        return -1;

    char *mem = kalloc();
    if (mem == 0)
        return -1;

    memset(mem, 0, PGSIZE);
    ilock(vma->file->ip);
    readi(vma->file->ip, 0, (uint64)mem, PGROUNDDOWN(va - vma->addr), PGSIZE);
    iunlock(vma->file->ip);

    int flags = PTE_U;
    if (vma->prot & PROT_WRITE)
        flags |= PTE_W;
    if (vma->prot & PROT_READ)
        flags |= PTE_R;
    if (mappages(pagetable, PGROUNDDOWN(va), PGSIZE, (uint64)mem, flags) != 0)
    {
        kfree(mem);
        return -1;
    }
    *pa = (uint64)mem;
    return 0;
}
