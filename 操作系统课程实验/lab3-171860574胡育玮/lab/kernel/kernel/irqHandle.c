#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

void GProtectFaultHandle(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallExec(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);

void irqHandle(struct StackFrame *sf) { // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	/*XXX Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch(sf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(sf);
			break;
		case 0x20:  //20:时钟中断
			timerHandle(sf);
			break;
		case 0x80:
			syscallHandle(sf);  //80：系统调用
			break;
		default:assert(0);
	}
	/*XXX Recover stackTop */  //恢复内核栈的栈顶
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf) 
{		
	int i;
	i = (current + 1) % MAX_PCB_NUM;
	// make blocked processes sleep time -1, sleep time to 0, re-run
	for (; i != current; i = (i + 1) % MAX_PCB_NUM)
	{
		if (pcb[i].state == STATE_BLOCKED && pcb[i].sleepTime > 0)
		{
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0)
				pcb[i].state = STATE_RUNNABLE; //如果sleeptime为0， 则转为就绪态
		}
	}

	// if time count not max, process continue
	pcb[current].timeCount++;
	if (pcb[current].timeCount < MAX_TIME_COUNT)
		return;  //直接回到doIrq中善后处理

	else
	{
		// else switch to another process
		/* echo pid of selected process */

		pcb[current].timeCount = 0;      //将本进程的timecount置为0
		pcb[current].state = STATE_RUNNABLE;  //状态从运行中改为就绪

		//然后切换进程
		for (i = (current + 1) % MAX_PCB_NUM; i != current; i = (i + 1) % MAX_PCB_NUM)
		{
			if (pcb[i].state == STATE_RUNNABLE)
			{
				//find = 1;
				break;
			}
		}
		
		current = i;
		pcb[current].state = STATE_RUNNING;			   //切换，开始工作
		putChar(pcb[current].pid + '0');				//输出进程号

		/*XXX recover stackTop of selected process */
		// setting tss for user process
		tss.esp0 = (uint32_t)(&pcb[current].stackTop); //设置tss状态段为核心栈栈顶

		// switch kernel stack：切换至核心栈
		asm volatile("movl %0, %%esp" ::"m"(pcb[current].stackTop));

		//这是原有的:弹出在进程控制块中保存的现场信息
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
}

void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case 0:
			syscallWrite(sf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(sf);
			break; // for SYS_FORK
		case 2:
			syscallExec(sf);
			break; // for SYS_EXEC
		case 3:
			syscallSleep(sf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(sf);
			break; // for SYS_EXIT
		default:break;
	}
}

void syscallWrite(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case 0:
			syscallPrint(sf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct StackFrame *sf) {
	int sel = sf->ds; //TODO segment selector for user data, need further modification
	char *str = (char*)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		if(character == '\n') {
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				displayRow=24;
				displayCol=0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==80){
				displayRow++;
				displayCol=0;
				if(displayRow==25){
					displayRow=24;
					displayCol=0;
					scrollScreen();
				}
			}
		}
		//asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		//asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
	return;
}

void syscallFork(struct StackFrame *sf) {
    // find empty pcb
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD)
			break;
	}
	if (i != MAX_PCB_NUM)  //找到了空的PCB，可以把复制后的进程拷贝到这里
	{
		/*XXX copy userspace
		  XXX enable interrupt
		 */
		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {  //拷贝用户空间
			*(uint8_t *)(j + (i+1)*0x100000) = *(uint8_t *)(j + (current+1)*0x100000);
			//asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		}
		/*XXX disable interrupt*/
		disableInterrupt();

		/*XXX set pcb 设置PCB的值
		  XXX pcb[i]=pcb[current] doesn't work不能直接进行结构体赋值
		*/
		for (j = 0; j < MAX_STACK_SIZE; j++)
			pcb[i].stack[j] = pcb[current].stack[j]; //拷贝栈

		pcb[i].stackTop = (uint32_t)(&pcb[i].regs); //设置栈顶:使得进程切换时能够读取保存在PCB中的现场信息
		pcb[i].prevStackTop = pcb[current].prevStackTop;

		pcb[i].state = STATE_RUNNABLE;				//进程状态为就绪
		pcb[i].timeCount = 0;						//时间片：0
		pcb[i].sleepTime = 0;						//睡眠时间：新fork出的进程没有睡眠
		pcb[i].pid = pcb[current].pid + 4;          //这个可以随意，不超出范围即可

		for (j = 0; j < 32; j++)                    //name字段的拷贝
			pcb[i].name[j] = pcb[current].name[j];

		/*XXX set regs */   //恢复之前保存到栈里的现场：使用sf
		pcb[i].regs.ss = USEL(2 * i + 2);
		pcb[i].regs.esp = sf->esp;
		pcb[i].regs.eflags = sf->eflags;
		pcb[i].regs.cs = USEL(2 * i + 1);
		pcb[i].regs.eip = sf->eip;

		pcb[i].regs.error = sf->error;
		pcb[i].regs.irq = sf->irq;

		//pcb[i].regs.eax = sf->eax;
		pcb[i].regs.ecx = sf->ecx;
		pcb[i].regs.edx = sf->edx;
		pcb[i].regs.ebx = sf->ebx;
		pcb[i].regs.xxx = sf->xxx;
		pcb[i].regs.ebp = sf->ebp;
		pcb[i].regs.esi = sf->esi;
		pcb[i].regs.edi = sf->edi;

		pcb[i].regs.ds = USEL(2 * i + 2);
		pcb[i].regs.es = USEL(2 * i + 2);
		pcb[i].regs.fs = USEL(2 * i + 2);
		pcb[i].regs.gs = USEL(2 * i + 2);

		/*XXX set return value */
		pcb[i].regs.eax = 0;       			//子进程:返回值为0
		pcb[current].regs.eax = pcb[i].pid; //父进程:为子进程的pid
	}

	else {
		pcb[current].regs.eax = -1;		//失败则返回-1
	}
	return;
}

void syscallExec(struct StackFrame *sf) {
	return;
}

void syscallSleep(struct StackFrame *sf) {
	pcb[current].state = STATE_BLOCKED;
	pcb[current].sleepTime = sf->ecx;

	//然后进程进程切换
	int i;
	int find = 0;
	for (i = (current + 1) % MAX_PCB_NUM; i != current; i = (i + 1) % MAX_PCB_NUM)
	{
		if (pcb[i].state == STATE_RUNNABLE)
		{
			find = 1;
			break;
		}
	}

	if (find)
		current = i;
	else
		current = 0; //IDLE

	pcb[current].state = STATE_RUNNING;             //切换，开始工作
	tss.esp0 = (uint32_t)(&pcb[current].stackTop);  //设置tss状态段

	//这是原有的:弹出在进程控制块中保存的现场信息
	asm volatile("movl %0, %%esp" ::"m"(pcb[current].stackTop)); 
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");

	return;
}

void syscallExit(struct StackFrame *sf) {
	pcb[current].state = STATE_DEAD;

	int i;
	int find = 0;
	for (i = (current + 1) % MAX_PCB_NUM; i != current; i = (i + 1) % MAX_PCB_NUM)
	{
		if (pcb[i].state == STATE_RUNNABLE)
		{
			find = 1;
			break;
		}
	}

	if (find)
		current = i;
	else
		current = 0; //IDLE

	pcb[current].state = STATE_RUNNING;			   //切换，开始工作
	tss.esp0 = (uint32_t)(&pcb[current].stackTop); //设置tss状态段

	//这是原有的:弹出在进程控制块中保存的现场信息
	asm volatile("movl %0, %%esp" ::"m"(pcb[current].stackTop));
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");

	return;
}
