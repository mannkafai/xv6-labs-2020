
struct vma
{
    uint64      addr;
    uint        length;
    int         prot;
    int         flags;
    struct file *file;
};
