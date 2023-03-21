/* Real Mode Hello World */
#.code16
#
#.global start
#start:
#	movw %cs, %ax
#	movw %ax, %ds
#	movw %ax, %es
#	movw %ax, %ss
#	movw $0x7d00, %ax
#	movw %ax, %sp         # setting stack pointer to 0x7d00
#	pushw $13             # pushing the size to print into stack
#	pushw $message        # pushing the address of message into stack
#	callw displayStr      # calling the display function to display "hello world"
#loop:
#	jmp loop  			  #jump to loop function
#
#message:
#	.string "Hello, World!\n\0"
#
#displayStr:
#	pushw %bp
#	movw 4(%esp), %ax
#	movw %ax, %bp
#	movw 6(%esp), %cx
#	movw $0x1301, %ax
#	movw $0x000c, %bx
#	movw $0x0000, %dx
#	int $0x10
#	popw %bp
#	ret



/* Protected Mode Hello World */
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	cli 								# clear interruption: 关闭中断
	inb $0x92, %al                      # Fast setup A20 Line with port 0x92, necessary or not?
	orb $0x02, %al
	outb %al, $0x92
	data32 addr32 lgdt gdtDesc          # loading gdtr, data32, addr32
	movl %cr0, %eax
	orb $0x01, %al  					# set last bit of cr0 to 1：设置CR0寄存器
	movl %eax, %cr0 					# setting cr0
	data32 ljmp $0x08, $start32         # reload code segment selector and ljmp to start32, data32



.code32
start32:
	movw $0x10, %ax       # setting data segment selector
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw $0x18, %ax       # setting graphics data segment selector
	movw %ax, %gs
	
	movl $0x8000, %eax    # setting esp
	movl %eax, %esp
   
    # jmp to bootMain in boot.c: 应该将下述代码修改为跳转到boot.c中的bootMain函数去执行
    jmp bootMain
    
#	pushl $13           #字符串中字符个数
#	pushl $message      #字符串首地址
#	calll displayStr    #调用字符串打印函数
#loop32:					#这个死循环很重要
#	jmp loop32




.p2align 2
gdt: 					# 8 bytes for each table entry, at least 1 entry
	.word 0,0           # empty entry
	.byte 0,0,0,0

	.word 0xffff,0      # code segment entry
	.byte 0,0x9a,0xcf,0

	.word 0xffff,0      # data segment entry
	.byte 0,0x92,0xcf,0

	.word 0xffff,0x8000 # graphics segment entry
	.byte 0x0b,0x92,0xcf,0

gdtDesc:                # 6 bytes in total
	.word (gdtDesc - gdt -1)        # size of the table, 2 bytes, 65536-1 bytes, 8192 entries
	.long gdt                       # offset, i.e. linear address of the table itself
