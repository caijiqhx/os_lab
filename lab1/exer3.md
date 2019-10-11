# EXER3: 分析 bootloader 进入保护模式的过程

BIOS 将通过读取硬盘主引导扇区到内存，并转跳到对应内存中的位置执行 bootloader。从练习 2 中我们知道从 `$cs=0 $ip=7c00` 开始即为 bootloader 的开始。下面结合 bootasm.S 和 gdb 调试分析从实模式进入保护模式的过程。
