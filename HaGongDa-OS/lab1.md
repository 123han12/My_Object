这篇文章将蓝桥云课中的实验1和实验二合并。

```
# 解压，并指定解压到 /home/shiyanlou/
# 这样的话，在 /home/shiyanlou/oslab/ 中就能找到解压后的所有文件
$ tar -zxvf hit-oslab-linux-20110823.tar.gz \
  -C /home/shiyanlou/
```



```
在多处理器的系统上，可以用 -j 参数进行并行编译，加快速度。例如双 CPU 的系统可以：
make -j 2
```



+ 文件交换流程

  ```
  oslab 下的 `hdc-0.11-new.img` 是 0.11 内核启动后的根文件系统镜像文件，相当于在 bochs 虚拟机里装载的硬盘。在 Ubuntu 上访问其内容的方法是：
  
  
  $ cd ~/oslab/
  # 启动挂载脚本
  $ sudo ./mount-hdc
  
  
  大家使用 sudo 时，password 是 shiyanlou，也有可能不会提示输入密码。
  
  
  之后，hdc 目录下就是和 0.11 内核一模一样的文件系统了，可以读写任何文件（可能有些文件要用 sudo 才能访问）
  
  # 进入挂载到 Ubuntu 上的目录
  $ cd ~/oslab/hdc
  
  # 查看内容
  $ ls -al
  
  读写完毕，不要忘了卸载这个文件系统：
  
  $ cd ~/oslab/
  
  # 卸载
  $ sudo umount hdc
  
  具体步骤：
  	首先通过 sudo ./mount-hdc 进行挂载。
  	然后在 Ubuntu 的 ~/oslab/hdc/usr/root 目录下创建一个 xxx.c 文件：sudo gedit ~/oslab/hdc/usr/root/xxx.c
  	最后执行 sudo umount hdc 后，再进入 Linux 0.11（即 run 启动 bochs 以后）就会看到这个 xxx.c（即如下图所示）。
  
  
  注意 1：不要在 0.11 内核运行的时候 mount 镜像文件，否则可能会损坏文件系统。同理，也不要在已经 mount 的时候运行 0.11 内核。
  
  注意 2：在关闭 Bochs 之前，需要先在 0.11 的命令行运行 “sync”，确保所有缓存数据都存盘后，再关闭 Bochs。
  
  
  ```



+ 两个命令编译和链接 `bootsect.s`

  ```
  as86 -0 -a -o bootsect.o bootsect.s
  ld86 -0 -s -o bootsect bootsect.o
  ```

  

+ ，要去掉这 32 个字节的文件头部（`tools/build.c` 的功能之一就是这个）！随手编个小的文件读写程序都可以去掉它。不过，懒且聪明的人会在` Ubuntu `下用命令

  ```
  dd bs=1 if=bootsect of=Image skip=32
  ```



```
回车符（Carriage Return）和换行符（Line Feed）都是常见的控制字符，用于控制文本的换行和回到行首的操作。它们在ASCII码中分别使用不同的数值表示：

回车符（Carriage Return）的ASCII码是13（十进制），表示为CR。

换行符（Line Feed）的ASCII码是10（十进制），表示为LF。

在文本编辑和处理中，通常会使用回车符和换行符的组合（CR + LF，即回车+换行）来表示一行的结束，这在许多操作系统和软件中都是标准的行结束符。

需要注意的是，不同的操作系统在处理文本行结束符的方式上可能存在差异。例如，Windows操作系统使用CR + LF作为行结束符，而Unix/Linux操作系统则通常只使用LF作为行结束符，而Macintosh操作系统在较早的版本中使用CR作为行结束符。这些差异可能会导致在不同操作系统间的文本文件在显示和处理上出现不兼容的情况，特别是在跨平台操作时。
```




**下面给出实验二的最终版的汇编语言的代码和详细的注释**

+ bootsect.s
```
SETUPLEN = 2 
SETUPSEG = 0x07e0
entry _start
_start:
	mov ah , #0x03
	xor bh, bh
	int 0x10
	mov cx , #36
	mov bx , #0x0007
	mov bp , #msg1
	mov ax , #0x07c0
	mov es , ax 
	mov ax , #0x1301 ; 显示字符
	int 0x10
load_setup:
    mov dx,#0x0000
    mov cx,#0x0002 ; 柱面为0 , 扇区为 2 
    mov bx,#0x0200
    mov ax,#0x0200+SETUPLEN   ; 注意这里直接将setup的程序加载到了0x07c0:0200处了，但在linux0.11中是加载到0x9000:0200中了。
    int 0x13
    jnc ok_load_setup  ; 没有进位的话跳转到ok_load_setup
    mov dx,#0x0000  ; 
    mov ax,#0x0000  ; 
    int 0x13
    jmp load_setup
ok_load_setup:
    jmpi    0,SETUPSEG ; 如果可以的话就直接跳转到0:SETUPSEG进行执行，相当于进入了setup的执行。
msg1:
    .byte   13,10
    .ascii  "Hello OS world, my name is LZJ"
    .byte   13,10,13,10
.org 510
boot_flag:
    .word   0xAA55
	
```

+ setup.s
```
INITSEG = 0x9000
entry _start
_start:
; Print "NOW we are in SETUP"
	mov ah, #0x03
	xor bh,bh
	int 0x10  ;功能号03表示读取光标位置，其中功能号在ah中，
	mov cx , #25  ;存储串长度
	mov bx,#0x0007 ;BH存储的是页号
	mov bp,#msg2 ; es:BP存储的是串地址
	mov ax,cs
	mov es,ax
	mov ax,#0x1301 ; ah存储的是功能号0x13 目的是显示字符串
	int 0x10
	
	mov ax , cs
	mov es , ax
	
; init ss:sp
	mov ax , #INITSEG
	mov ss , ax 
	mov sp, #0xFF00   ; 0xFF00的值为  65285字节，栈顶指针相当于在9000:FF00 处了，注意栈是从高地址向底地址进行增长的。
	
; Get Params , 得到第一个硬盘的参数表
	mov ax,#INITSEG
	mov ds,ax ;源段地址为0x9000
	mov ah,#0x03 ; 功能号为 #0x03，功能为读取光标位置 ， CH为光标开始行，CL为光标结束行，DH存储行，DL存储列。
	xor bh , bh  ; xor是按位异或的指令，相当于对bh进行一个清零了，表示第零页
	int 0x10
	mov [0] , dx; 将光标所在的行和列存储到ds:0所在的字单元
	mov ah,#0x88 ; 在0x15中断下的88号功能：读取扩展内存大小 
	int 0x15 
	mov [2] , ax ; 出口参数是AX = 扩展内存字节数(以K为单位) 
	mov ax , #0x0000
	mov ds , ax
	lds si , [4*0x41] ; 将第一个硬盘参数表的偏移地址放到si中 	
	; 用32位或48位的数据加载16位DS段选择符和16或32位的通用寄存器值，其中通用寄存器在目的操作数中。
	;LDS (16or32)通用寄存器, (32or48)内存变量的地址
	
	mov ax , #INITSEG
	mov es , ax 
	mov di , #0x0004
	mov cx , #0x10 
	rep movsb  ; “rep movsb” 指令使用 “rep” 前缀来表示重复执行，“movsb” 是 move string byte 的缩写，用于按字节（byte）复制字符串。
	
; Be Ready to Print
	mov ax , cs  ; cs为0x07c0
	mov es , ax  ; 
	mov ax , #INITSEG
	mov ds , ax 

; Cursor position
	mov ah , #0x03
	xor bh , bh ; 这里进行了清空，方便后面的中断的执行
	int 0x10    ; 执行中断之后，DH存储行，DL存储列。
	mov cx , #18
	mov bx , #0x0007
	mov bp , #msg_cursor
	mov ax , #0x1301   ; 在屏幕输出"Cursor position:" , 入口参数 AH = 13H , BH = 页码 ， CX = 显示的字符串的长度，(DH , DL) = (行，列)
	; ES:BP = 显示字符串地址，AL = 显示输出方式
	int 0x10
	mov dx , [0]  ; 得到放在0x07c0处的鼠标位置的参数。
	call print_hex

; Memory Size
	mov ah , #0x03
	xor bh , bh
	int 0x10  ;  执行中断之后，DH存储行，DL存储列
	mov cx , #14
	mov bc , #0x0007
	mov bp , #msg_memory
	mov ax , #0x1301
	int 0x10 
	mov dx , [2]
	call print_hex 

; Add KB
    mov ah,#0x03
    xor bh,bh 
    int 0x10 ;  执行中断之后，DH存储行，DL存储列
    mov cx,#2
    mov bx,#0x0007
    mov bp,#msg_kb
    mov ax,#0x1301
    int 0x10  ; 直接打印出 KB
; Cyles
    mov ah,#0x03
    xor bh,bh
    int 0x10
    mov cx,#7
    mov bx,#0x0007
    mov bp,#msg_cyles
    mov ax,#0x1301
    int 0x10
    mov dx,[4]  ; 第一个硬盘参数是柱面，占一个字
    call    print_hex
; Heads
    mov ah,#0x03
    xor bh,bh
    int 0x10
    mov cx,#8
    mov bx,#0x0007
    mov bp,#msg_heads
    mov ax,#0x1301
    int 0x10
    mov dx,[6] ; 第二个参数为磁头数，占一个字节。
    call    print_hex


;.......中间省略好多参数，这里直接到了:
; Secotrs  磁头着陆柱面号，占一个字。
    mov ah,#0x03
    xor bh,bh
    int 0x10
    mov cx,#10
    mov bx,#0x0007
    mov bp,#msg_sectors
    mov ax,#0x1301
    int 0x10
    mov dx,[12]
    call    print_hex





inf_loop: ; 死循环
	jmp inf_loop 
print_hex:
	mov cx , #4 ; 将cx的值置为四
print_digit:
	rol dx , #4 ; 用于将寄存器 DX 的值向左循环移位（rotate）4位。，之后最低的四位就是原本最高的四位了。
	mov ax , #0xe0f ; ax 中存储的值相当于:  0000 1110 0000 1111
	and al , dl  ; 获取最低四位的值，放到al中。
	add al , #0x30   ;转换为其ascii码的值。
	cmp al , #0x3a  ; 如果大于10了，我们需要额外的加上 7 
	jl outp
	add al , #0x07 ;说明数字大于10了，需要加上额外的ascii值。
outp:
	int 0x10   ; 触发0x10中断，功能号为0x0eH , 显示字符，入口参数 AH = 0EH , AL = 字符， BH = 页码
	loop print_digit 
	ret 
print_nl:
	mov ax , #0xe0d
	int 0x10
	mov al , #0xa
	int 0x10
	ret 
	











msg2: ;这一段的大小为25bytes
	.byte 13,10
	.ascii "NOW we are in SETUP"
	.byte 13,10,13,10

msg_cursor:
	.byte 13,10
	.ascii "Cursor position:"

msg_memory:
	.byte 13,10
	.ascii "Memory Size:"
	
msg_cyles:
	.byte 13,10
	.ascii "Cyls:"

msg_heads:
	.byte 13,10
	.ascii "head:"
msg_sectors:
	.byte 13,10
	.ascii "Sectors:"
msg_kb:
	.ascii "KB"
.org 510
boot_flag:
	.word 0xAA55

```














