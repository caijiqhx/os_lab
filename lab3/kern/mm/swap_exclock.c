#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_exclock.h>
#include <list.h>

list_entry_t pra_list_head, *clock;
uint32_t judge;

static int
_exclock_init_mm(struct mm_struct *mm) {
    list_init(&pra_list_head);
    clock = mm->sm_priv = &pra_list_head;
    judge = 0;
    //cprintf(" mm->sm_priv %x in exclock_init_mm\n",mm->sm_priv);
    return 0;
}

static int
_exclock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in) {
    if(judge) {
        return 0;
    }
    list_entry_t *head = (list_entry_t*) mm->sm_priv;
    list_entry_t *entry = &(page->pra_page_link);

    assert(entry != NULL && head != NULL);
    list_add_before(head, entry);
    return 0;
}

// 遍历循环链表找到要换出的页
static int
_exclock_swap_out_victim(struct mm_struct *mm, struct Page **ptr_page, int in_trick) {
    judge = 1;
    list_entry_t *head = (list_entry_t*) mm->sm_priv;
    assert(clock != NULL);
    assert(in_trick == 0);

    // Select the victim
    for(; ; clock = list_next(clock)){
        if(clock == head) {
            continue;
        }
        struct Page *victim = le2page(clock, pra_page_link);
        pte_t *ptep = get_pte(mm->pgdir, victim->pra_vaddr, 0);
        assert(*ptep & PTE_P);
        uintptr_t a = *ptep & PTE_A, d = *ptep & PTE_D;
        cprintf("visit vaddr: 0x%08x %c%c\n", victim->pra_vaddr, a ? 'A' : '-', d ? 'D' : '-');
        if (a == 0 && d == 0) { // 0/0 -> swap out
            *ptr_page = victim;
            break;    
        }else {
            if (a == 0) { // 0/1 -> 0/0 将页面写入 swap 分区
                if (swapfs_write((victim->pra_vaddr / PGSIZE + 1) << 8, victim) == 0) {
                    cprintf("write page vaddr 0x%x to swap %d\n", victim->pra_vaddr, victim->pra_vaddr / PGSIZE + 1);
                    *ptep &= ~PTE_D;
                } 
            } else { // 1/0, 1/1 -> 0/0, 0/1
                *ptep &= ~PTE_A;
            }
            tlb_invalidate(mm->pgdir, victim->pra_vaddr); // 刷新快表
        }
    }

    clock = list_next(clock);
    return 0;
}

/* check 程序按照视频课程中演示的样例
 *
 * 内存中的页面初始状态, page(A/D):
 * a(0/0), b(0/0), c(0/0), d(0/0) <==> ^a00, b00, c00, d00
 * 
 * 要注意的是 clock 指针的位置，用 ^ 表示
 * 
 * 下面开始一系列访问请求：
 * 1. read c: ^a00, b00, c10, d00
 * 2. write a: ^a11, b00, c10, d00
 * 3. read d: ^a11, b00, c10, d10
 * 4. write b: ^a11, b11, c10, d10
 * 5. read e: page fault -> exclock -> a00, b00, e10, ^d00
 * 6. read b: a00, b10, e10, ^d00
 * 7. write a: a11, b10, e10, ^d00
 * 8. read b: a11, b10, e10, ^d00
 * 9. read c: page fault -> exclock -> ^a11, b10, e10, c10
 * 10. read d: page fault -> exclock -> a00, d10, ^e00, c00
 * 
 */

static int
_exclock_check_swap(void) {
    int i;

    // 首先将页的访问位和修改位都置0
    pde_t *pgdir = KADDR((pde_t*) rcr3());
    for(i = 1; i < 5; i++ ) {
        pte_t *ptep = get_pte(pgdir, i * 0x1000, 0);
        swapfs_write((i * 0x1000 / PGSIZE + 1) << 8, pte2page(*ptep));
        *ptep &= ~(PTE_A | PTE_D);
        tlb_invalidate(pgdir, i * 0x1000);
    }

    // 模拟演示样例
    assert(pgfault_num == 4);
    cprintf("read Page c in exclock_check_swap\n");
    assert(*(unsigned char *)0x3000 == 0x0c);
    
    assert(pgfault_num == 4);
    cprintf("write Page a in exclock_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0xa;

    assert(pgfault_num == 4);
    cprintf("read Page d in exclock_check_swap\n");
    assert(*(unsigned char *)0x4000 == 0x0d);

    assert(pgfault_num == 4);
    cprintf("write Page b in exclock_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    *(unsigned char *)0x2000 = 0x0b;

    assert(pgfault_num == 4);
    cprintf("read Page e in exclock_check_swap\n");
    unsigned e = *(unsigned char *)0x5000;
    cprintf("e = 0x%04\n", e);

    assert(pgfault_num == 5);
    cprintf("read Page b in exclock_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);

    assert(pgfault_num == 5);
    cprintf("write Page a in exclock_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;

    assert(pgfault_num == 5);
    cprintf("read Page b in exclock_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);

    assert(pgfault_num == 5);
    cprintf("read Page c in exclock_check_swap\n");
    assert(*(unsigned char *)0x3000 == 0x0c);

    assert(pgfault_num == 6);
    cprintf("read Page d in exclock_check_swap\n");
    assert(*(unsigned char *)0x4000 == 0x0d);
    
    assert(pgfault_num == 7);

    return 0;
}

static int 
_exclock_init(void) {
    return 0;
}

static int 
_exclock_set_unswappable(struct mm_struct *mm, uintptr_t addr) {
    return 0;
}

static int
_exclock_tick_event(struct mm_struct *mm) {
    return 0;
}

struct swap_manager swap_manager_exclock = {
    .name            = "exclock swap manager",
     .init            = &_exclock_init,
     .init_mm         = &_exclock_init_mm,
     .tick_event      = &_exclock_tick_event,
     .map_swappable   = &_exclock_map_swappable,
     .set_unswappable = &_exclock_set_unswappable,
     .swap_out_victim = &_exclock_swap_out_victim,
     .check_swap      = &_exclock_check_swap,
};