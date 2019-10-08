# EXER1: 理解通过 make 生成执行文件的过程

> 首先回顾一下 make，在 lab0 的已有对 make 的介绍，另有[gnu 的官方文档](https://www.gnu.org/software/make/manual/)。
>
> make 是一个命令工具，make 命令执行时，需要一个 makefile 文件来告诉 make 以何种方式编译源代码和链接程序。
>
> make 编译链接文件的规则：
>
> - 所有的源文件没有被编译过，则对各个源文件进行编译和链接，生成最后的可执行程序；
> - 每一个在上次执行 make 之后修改过的源文件在本次执行 make 时将会被重新编译；
> - 头文件在上一次执行 make 之后被修改，则包含此头文件的源文件在本次执行 make 时将会被重新编译。
>
> 一个简单的 makefile 描述规则组成：
>
> ```makefile
> target... : prerequisites...
> 	command
> 	...
> 	...
> ```
>
> - target：规则的目标。通常是最后生成的文件名或者为了实现这个目的而必须的中间过程文件名。可以是.o 文件、也可以是最后的可执行程序的文件名等。另外，目标也可以是一个 make 执行的动作的名称，如 “clean”，称为伪目标。
> - prerequisites：规则的依赖。生成规则目标所需要的文件名或目标列表。通常一个目标依赖于一个或者多个文件。
> - command：规则的命令行。规则所要执行的动作（任意的 shell 命令或者是可在 shell 下执行的程序）。
>
> 当规则的目标是一个文件，在它的任何一个依赖文件被修改后，在执行“make”时这个目标文件将会被重新编译或重新链接。

## Q1: 操作系统镜像文件 `ucore.img` 是如何一步一步生成的?

在 makefile 中找到 `ucore.img` 相关代码：

```makefile
V       := @
...
include tools/function.mk
...
# create ucore.img
UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)
```

首先调用了 call 函数, 将 ucore.img 传入了 totarget 表达式，结果赋值给 UCOREIMG，这里的 totarget 表达式是在包含的文件 tools/function.mk 文件中（找了半天 tcl：

```makefile
totarget = $(addprefix $(BINDIR)$(SLASH),$(1))
```

所以最后赋值给 UCOREIMG 的值为 bin/ucore.img，即目标镜像文件。

可以看到镜像的依赖有 bootblock 、kernel，依赖生成后，执行下面的命令。

\$(V) 即 '@'，表示在执行命令时不会输出命令，'\$@' 表示目标文件即 ucore.img ， dd 命令创建了一个大小为 10000×512 字节的空文件，然后将 bootblock 赋值到文件的第一个块，将 kernel 跳过第一块复制到后续的块中。

> 这里看到一个 `dd` 命令参数 `conv=notrunc`，[参考](https://stackoverflow.com/questions/20526198/why-using-conv-notrunc-when-cloning-a-disk-with-dd) > </br>大概就是： notrunc is only important to prevent truncation when writing into a file.
