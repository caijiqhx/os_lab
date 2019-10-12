# Lab1 report

## 2019-10-09:16:26

操作系统课上，练习 1 完成。

以前只写过简单的 makefile，这个 makefile 看了很久。。。

实践证明以后分析这玩意就一顿 warning 输出就完事了。

## 2019-10-11:19:35

练习 2 完成。

实践证明 gdb 也不是很靠谱，BIOS 的第一条指令被反汇编成了 `ljmp 0x3630, 0xf000e05b`，看了半天才发现。。。

这里在开始写练习 2 记录的时候想了一会，问了一下同学，他说他的反汇编正常并认为我的 gdb 有问题，但是我在 wsl、virtualbox、aliyunecs 等多个 ubuntu1804 环境下测试后结果都相同，并未找到问题所在。

过了一天后我在 stackoverflow 上搜了一下[关于 gdb 反汇编 16 位指令的问题](https://stackoverflow.com/questions/32955887/how-to-disassemble-16-bit-x86-boot-sector-code-in-gdb-with-x-i-pc-it-gets-tr/32960272)，发现其他人也有相关问题。其中有一个回答给出了一个 target.xml 文件，指出在 gdb 中使用命令 set tdesc filename target.xml 覆盖 gdb 的设置。经测试，此方法确实可以调整反汇编出 `ljmp $0xf000,$0xe05b` 命令。给出此文件的人认为是 gdb 新版本处理的问题，具体原因等我写完所有实验再探究一下。

## 2019-10-12:16:45

练习 3 完成。

实验三难度不是很大，主要就是查了一些汇编以及关于 GDT 表的内容。
