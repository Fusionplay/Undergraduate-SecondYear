.code32
.global start

message:
	.string "Hello, Worddld!"

start:
	pushl $15
	pushl $message
	calll displayStr
	
loop32:
	jmp loop32

displayStr:
	movl 4(%esp), %ebx
	movl 8(%esp), %ecx
	movl $((80*5+0)*2), %edi
	movb $0x0c, %ah
	
	
nextChar:
	movb (%ebx), %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	incl %ebx
	loopnz nextChar     # loopnz decrease ecx by 1
	ret
	
	

