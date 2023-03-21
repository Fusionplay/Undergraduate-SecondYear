#include "pthread.h"
#include "lib.h"
/*
 * pthread lib here
 * 用户态多线程写在这
 */
 
ThreadTable tcb[MAX_TCB_NUM];
int current;

void pthread_initial(void)
{
    int i;
    for (i = 0; i < MAX_TCB_NUM; i++) {
        tcb[i].state = STATE_DEAD;
        tcb[i].joinid = -1;
    }
    tcb[0].state = STATE_RUNNING;
    tcb[0].pthid = 0;
    current = 0; // main thread
    return;
}


int schedule()
{
	int i;
	i = (current + 1) % MAX_TCB_NUM;
	for (; i != current; i = (i + 1) % MAX_TCB_NUM)
	{
		if (tcb[i].state == STATE_RUNNABLE)
			break;
	}

	if (i == current && tcb[i].state != STATE_RUNNABLE)
		return -1;

	current = i;
	tcb[current].state = STATE_RUNNING;


	asm volatile("movl %0, %%edi" ::"m"(tcb[current].cont.edi));
	asm volatile("movl %0, %%esi" ::"m"(tcb[current].cont.esi));
	asm volatile("movl %0, %%ebx" ::"m"(tcb[current].cont.ebx));
	asm volatile("movl %0, %%edx" ::"m"(tcb[current].cont.edx));
	asm volatile("movl %0, %%ecx" ::"m"(tcb[current].cont.ecx));
	asm volatile("movl %0, %%eax" ::"m"(tcb[current].cont.eax));

	asm volatile("movl %0, %%ebp" ::"m"(tcb[current].cont.ebp));
	asm volatile("movl %0, %%esp" ::"m"(tcb[current].cont.esp));
	//asm volatile("movl %0, %%eax" ::"m"(tcb[current].cont.eip));
	//asm volatile("jmp *%eax");
	asm volatile("jmp *%0" ::"m"(tcb[current].cont.eip));

	return 5;
}


int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{	//todo:
	//1. set cpu.eip of the start_routine
	//2. push arg to the stack top
	/*
	创建一个新的线程，使之进入runnable状态。
	应当做到：当这个线程在后面被调度到之后，能马上开始运行：也即执行这个线程的函数。
	问题：函数执行完后，返回到哪里去？
	*/
	
	int i = 0;
	for (; i < MAX_TCB_NUM; i++)
	{
		if (tcb[i].state == STATE_DEAD)
			break;
	}

	if (i == MAX_TCB_NUM)
		return -1;

	tcb[i].state = STATE_RUNNABLE;
	*thread = i;
	tcb[i].pthArg = (uint32_t)arg;
	tcb[i].pthid = i;
	//tcb[i].stackTop = (uint32_t)(&tcb[i].stack[1535]);
	tcb[i].stack[1535] = (uint32_t)arg; //参数入栈
	//tcb[i].cont.esp = (uint32_t)(&tcb[i].stack[1535]);
	tcb[i].cont.ebp = (uint32_t)(&tcb[i].stack[1534]);
	tcb[i].cont.esp = (uint32_t)(&tcb[i].stack[1534]);
	//asm volatile("movl %0, %%eax" ::"m"(start_routine));
	tcb[i].cont.eip = (uint32_t)start_routine;
	//asm volatile("jmp *%eax");

	return 0;
}

void pthread_exit(void *retval)
{
	/*
	1. 将自己的状态置为死亡
	2. 逐个查找线程表，看有那些进程join了自己：将他们置为就绪态
	*/
	tcb[current].state = STATE_DEAD;

	int i;
	for (i = 0; i < MAX_TCB_NUM; i++)
	{
		if (tcb[i].joinid == tcb[current].pthid && tcb[i].state == STATE_BLOCKED)
			tcb[i].state = STATE_RUNNABLE;
	}

	schedule();
	// int val = schedule(0);
	// if (val != -1)
		//recover();

	return;
}


int pthread_join(pthread_t thread, void **retval)
{
	//printf("In Join\n");
	//if (tcb[thread].state == STATE_DEAD && tcb[current].state == STATE_BLOCKED)
	if (tcb[thread].state == STATE_DEAD)
	{   //要等待的进程的已经死亡：此时将自己的状态改为可以运行，然后调度别的进程上来
		// tcb[current].state = STATE_RUNNABLE;
		//调度
		int val = schedule();
		if (val == -1)
			return -1;
		//printf("\n");
	}

	else
	{
		//要等待的进程的未死亡：此时将自己的joinid设为要等的进程的ID，然后保存现场，阻塞自己，调度别的进程上来做
		asm volatile("movl %%edi, %0" ::"m"(tcb[current].cont.edi));
		asm volatile("movl %%esi, %0" ::"m"(tcb[current].cont.esi));
		asm volatile("movl %%ebx, %0" ::"m"(tcb[current].cont.ebx));
		asm volatile("movl %%edx, %0" ::"m"(tcb[current].cont.edx));
		asm volatile("movl %%ecx, %0" ::"m"(tcb[current].cont.ecx));
		asm volatile("movl %%eax, %0" ::"m"(tcb[current].cont.eax));

		asm volatile("movl (%%ebp), %0" : "=a"(tcb[current].cont.ebp));
		asm volatile("leal 8(%%ebp), %0" : "=a"(tcb[current].cont.esp));
		asm volatile("movl 4(%%ebp), %0" : "=a"(tcb[current].cont.eip));

		tcb[current].state = STATE_BLOCKED; //阻塞自己
		tcb[current].joinid = thread;

		int val = schedule();				//不包括cur
		if (val == -1)
			return -1;
	}

	return 0;
}

int pthread_yield(void)
{
	//切换：1.保存现场：2.选择一个阻塞的线程 3. 恢复这个线程的现场
	//1. 保存现场：把相应的值放到当前进程的context里
	/*
	struct Context
	{
		uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
		uint32_t eip;
	};
	*/
	asm volatile("movl %%edi, %0" ::"m"(tcb[current].cont.edi));
	asm volatile("movl %%esi, %0" ::"m"(tcb[current].cont.esi));
	asm volatile("movl %%ebx, %0" ::"m"(tcb[current].cont.ebx));
	asm volatile("movl %%edx, %0" ::"m"(tcb[current].cont.edx));
	asm volatile("movl %%ecx, %0" ::"m"(tcb[current].cont.ecx));
	asm volatile("movl %%eax, %0" ::"m"(tcb[current].cont.eax));

	asm volatile("movl (%%ebp), %0" :"=a"(tcb[current].cont.ebp));
	asm volatile("leal 8(%%ebp), %0" :"=a"(tcb[current].cont.esp));
	asm volatile("movl 4(%%ebp), %0" :"=a"(tcb[current].cont.eip));

	tcb[current].state = STATE_RUNNABLE;

	//2.选择一个阻塞的线程
	//3. 恢复这个线程的现场
	int val = schedule(); //不包括cur
	if (val == -1)
		return -1;


	return 0;
}