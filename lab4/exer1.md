# EXER1: 分配并初始化一个进程控制块

实现 alloc_proc 函数，负责分配并返回一个新的 struct proc_struct 结构，用于存储新建立的内核线程的管理信息。

```C
// alloc_proc - alloc a proc_struct and init all fields of proc_struct
static struct proc_struct *
alloc_proc(void) {
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
    //LAB4:EXERCISE1 YOUR CODE
    /*
     * below fields in proc_struct need to be initialized
     *       enum proc_state state;                      // Process state
     *       int pid;                                    // Process ID
     *       int runs;                                   // the running times of Proces
     *       uintptr_t kstack;                           // Process kernel stack
     *       volatile bool need_resched;                 // bool value: need to be rescheduled to release CPU?
     *       struct proc_struct *parent;                 // the parent process
     *       struct mm_struct *mm;                       // Process's memory management field
     *       struct context context;                     // Switch here to run process
     *       struct trapframe *tf;                       // Trap frame for current interrupt
     *       uintptr_t cr3;                              // CR3 register: the base addr of Page Directroy Table(PDT)
     *       uint32_t flags;                             // Process flag
     *       char name[PROC_NAME_LEN + 1];               // Process name
     */
        memset(proc, 0, sizeof(struct proc_struct));     // 结构体中的大多数成员变量在初始化时置 0 即可
        proc->state = PROC_UNINIT;                       // 进程状态设置为 PROC_UNINIT，其实这个值本来就是 0，这句不写也行
        proc->pid = -1;                                  // pid 赋值为 -1，表示进程尚不存在
        proc->cr3 = boot_cr3;                            // 内核态进程的公用页目录表
    }
    return proc;
}
```

## Q: 请说明 proc_struct 中 struct context context 和 struct trapframe \*tf 成员变量含义和在本实验中的作用是啥？

context 即进程上下文，保存了进程切换前的寄存器

```C
struct context {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
};
```
