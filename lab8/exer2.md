# EXER2: 完成基于文件系统的执行程序机制的实现

改写 ./kern/process/proc.c 中的 load_icode 函数，实现基于文件系统的执行程序机制。

大多数的代码和 lab5 相似，有两处不同，从 fd 中读取文件数据

```C
static int
load_icode(int fd, int argc, char **kargv) {
    if (current->mm != NULL) {
        panic("load_icode: current->mm must be empty.\n");
    }

    int ret = -E_NO_MEM;
    struct mm_struct *mm;
    //(1) create a new mm for current process
    if ((mm = mm_create()) == NULL) {
        goto bad_mm;
    }
    //(2) create a new PDT, and mm->pgdir= kernel virtual addr of PDT
    if (setup_pgdir(mm) != 0) {
        goto bad_pgdir_cleanup_mm;
    }
    //(3) copy TEXT/DATA/BSS parts in binary to memory space of process
    struct Page *page;
    //(3.1) read raw data content in file and resolve elfhdr
    struct elfhdr elf;
    if (ret = load_icode_read(fd, &elf, sizeof(struct elfhdr), 0) != 0)
        goto bad_elf_cleanup_pgdir;
    if (elf.e_magic != ELF_MAGIC) {
        ret = -E_INVAL_ELF;
        goto bad_elf_cleanup_pgdir;
    }
    // (3.2) read raw data content in file and resolve proghdr based on info in elfhdr
    uint32_t vm_flags, perm, phnum;
    struct proghdr __ph, *ph = &__ph;
    for (phnum = 0; phnum < elf.e_phnum; ++phnum) {
        if ((ret = load_icode_read(fd, ph, sizeof(struct proghdr), elf.e_phoff + sizeof(struct proghdr) * phnum)) != 0)
            goto bad_cleanup_mmap;
        if (ph->p_type != ELF_PT_LOAD) {
            continue;
        }
        if (ph->p_filesz > ph->p_memsz) {
            ret = -E_INVAL_ELF;
            goto bad_cleanup_mmap;
        }
        if (ph->p_filesz == 0) {
            continue;
        }
    //(3.3) call mm_map to build vma related to TEXT/DATA
        vm_flags = 0, perm = PTE_U;
        if (ph->p_flags & ELF_PF_X) vm_flags |= VM_EXEC;
        if (ph->p_flags & ELF_PF_W) vm_flags |= VM_WRITE;
        if (ph->p_flags & ELF_PF_R) vm_flags |= VM_READ;
        if (vm_flags & VM_WRITE) perm |= PTE_W;
        if ((ret = mm_map(mm, ph->p_va, ph->p_memsz, vm_flags, NULL)) != 0) {
            goto bad_cleanup_mmap;
        }
        off_t from = ph->p_offset;
        size_t off, size;
        uintptr_t start = ph->p_va, end, la = ROUNDDOWN(start, PGSIZE);

        ret = -E_NO_MEM;

    // (3.4) callpgdir_alloc_page to allocate page for TEXT/DATA, read contents in file and copy them into the new allocated pages
        end = ph->p_va + ph->p_filesz;
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PGSIZE - off, la += PGSIZE;
            if (end < la) {
                size -= la - end;
            }
            if ((ret = load_icode_read(fd, page2kva(page) + off, size, from)) != 0)
                goto bad_cleanup_mmap;
            // memcpy(page2kva(page) + off, from, size);
            start += size, from += size;
        }
    // (3.5) callpgdir_alloc_page to allocate pages for BSS, memset zero in these pages
        end = ph->p_va + ph->p_memsz;
        if (start < la) {
            /* ph->p_memsz == ph->p_filesz */
            if (start == end) {
                continue ;
            }
            off = start + PGSIZE - la, size = PGSIZE - off;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
            assert((end < la && start == end) || (end >= la && start == la));
        }
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PGSIZE - off, la += PGSIZE;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
        }
    }
    sysfile_close(fd);
    //(4) call mm_map to setup user stack, and put parameters into user stack
    vm_flags = VM_READ | VM_WRITE | VM_STACK;
    if ((ret = mm_map(mm, USTACKTOP - USTACKSIZE, USTACKSIZE, vm_flags, NULL)) != 0) {
        goto bad_cleanup_mmap;
    }
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-PGSIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-2*PGSIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-3*PGSIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-4*PGSIZE , PTE_USER) != NULL);

    //(5) setup current process's mm, cr3, reset pgidr (using lcr3 MARCO)
    mm_count_inc(mm);
    current->mm = mm;
    current->cr3 = PADDR(mm->pgdir);
    lcr3(PADDR(mm->pgdir));

    //(7) setup trapframe for user environment
    struct trapframe *tf = current->tf;
    memset(tf, 0, sizeof(struct trapframe));
    tf->tf_cs = USER_CS;
    tf->tf_ds = tf->tf_es = tf->tf_ss = USER_DS;
    tf->tf_esp = USTACKTOP;
    tf->tf_eip = elf.e_entry;
    tf->tf_eflags = FL_IF;

    // (6) setup uargc and uargv in user stacks
    // push raw data into stack
    char* uargv[EXEC_MAX_ARG_NUM];
    uargv[argc] = NULL;
    for (int i = argc - 1; i >= 0; --i) {
        tf->tf_esp -= strlen(kargv[i]) + 1;
        uargv[i] = (char*) tf->tf_esp;
        strcpy(uargv[i], kargv[i]);
    }
    // push char* pointer into stack
    tf->tf_esp = ROUNDDOWN(tf->tf_esp, sizeof(uintptr_t)) - sizeof(char*) * (argc + 1);
    memcpy((char**)tf->tf_esp, uargv, sizeof(char*) * (argc + 1));
    // push argc
    tf->tf_esp -= sizeof(uintptr_t);
    *(uintptr_t*)tf->tf_esp = argc;

    ret = 0;
out:
    return ret;
bad_cleanup_mmap:
    exit_mmap(mm);
bad_elf_cleanup_pgdir:
    put_pgdir(mm);
bad_pgdir_cleanup_mm:
    mm_destroy(mm);
bad_mm:
    goto out;
}
```
