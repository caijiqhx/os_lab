# lab2 report

## 2019-10-29:01:52

练习 1 完成。过程比较简单，主要是要看明白参考资料的一些先导知识，见 preview.md。

主要函数在 ./kern/mm/default_pmm.c 中，初步代码已经提供，稍加修改即可。

在代码中用到了一个 le2page 宏，通过 page_link 成员找到对应的 Page 结构体：

```C
// 获取 type 结构体中成员 member 的偏移地址，这种用法好像在 linux 源码里有，tql
#define offsetof(type, member)                                      \
    ((size_t)(&((type *)0)->member))

// 用 page_link 的地址减去其在 Page 结构体中的偏移就获取了 Page 结构体的地址，相当于 container_of
#define to_struct(ptr, type, member)                               \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define le2page(le, member)                 \
    to_struct((le), struct Page, member)
```

## 2019-10-30:23:31

练习 2 完成。

主要是要先了解分页机制和 ucore 中进行地址映射的几个阶段。见 preview.md。

要实现的函数就是第三个阶段的 boot_map_segment 函数调用的 get_pte。

ucore 的内核虚拟地址空间如下：

```
/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xFB000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PTSIZE
 *     VPT -----------------> +---------------------------------+ 0xFAC00000
 *                            |        Invalid Memory (*)       | --/--
 *     KERNTOP -------------> +---------------------------------+ 0xF8000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE
 *                            |                                 |
 *     KERNBASE ------------> +---------------------------------+ 0xC0000000
 *                            |                                 |
 *                            |                                 |
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *
 * */
```
