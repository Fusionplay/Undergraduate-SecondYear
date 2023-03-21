#include "lib.h"
#include "types.h"

int data = 0;

/*
int uEntry(void)
{    
    int i = 4;
    int ret = 0;
    sem_t sem;
    printf("Father Process: Semaphore Initializing.\n");
    ret = sem_init(&sem, 2);
    if (ret == -1) {
        printf("Father Process: Semaphore Initializing Failed.\n");
        exit();
    }

    ret = fork();
    if (ret == 0) 
	{
        while (i != 0) 
		{
            i --;
            printf("Child Process: Semaphore Waiting.\n");
            sem_wait(&sem);
            printf("Child Process: In Critical Area.\n");
        }

        printf("Child Process: Semaphore Destroying.\n");
        sem_destroy(&sem);
        exit();
    }

    else if (ret != -1) 
	{
        while( i != 0) 
		{
            i --;
            printf("Father Process: Sleeping.\n");
            sleep(128);
            printf("Father Process: Semaphore Posting.\n");
            sem_post(&sem);
        }

        printf("Father Process: Semaphore Destroying.\n");
        sem_destroy(&sem);
        exit();
    }

	return 0;
}
*/



int uEntry()
{
	int ret = 0;
	sem_t product;
	sem_t access;
	sem_t empty;
	sem_t full;
	printf("Main Process: Semaphore Initializing.\n");
	ret = sem_init(&product, 0);
	ret = sem_init(&access, 1);
	ret = sem_init(&empty, 8);
	ret = sem_init(&full, 0);
	int pid = -1;
	if (ret == -1)
	{
		printf("Main Process: Semaphore Initializing Failed.\n");
		exit();
	}

	int i;
	for (i = 0; i < 6; i++)
	{
		ret = fork();
		if (ret == 0)
		{
			pid = getpid();
			printf("pid==========%d\n", pid);
			//sleep(128);
			break;
		}
	}
	

	//每一行输出需要表达pid i, producer/consumer j, operation(, product k)
	//operation包括try lock、locked、unlock、produce、try consume、consumed
	
	/*
	if (pid <= 3 && pid > 1) //producer
	{
		//printf("OK here\n");
		//int z = 0;
		int num = 0;
		//while (z<8)
		while (1)
		{
			num += 8;
			printf("pid: %d, producer: %d, op: produce %d\n", pid, pid, num);

			printf("pid: %d, producer: %d, op: try lock %d\n", pid, pid, num);
			sem_wait(&access);

			printf("pid: %d, producer: %d, op: locked\n", pid, pid);
			int j;
			for (j = 0; j < 2; j++)
				sem_post(&product);
			//++z;
			sem_post(&access);
			printf("pid: %d, producer: %d, op: unlock\n", pid, pid);

			sleep(64);
		}
		//exit();
	}
	*/
	if (pid <= 3 && pid > 1) //producer
	{
		//printf("OK here\n");
		int z;
		int num = 0;
		
		while (1)
		{
			z = 0;
			while (z < 8)
			{
				num += 1;
				printf("pid: %d, producer: %d, op: produce %d\n", pid, pid, num);
				sem_wait(&empty);

				printf("pid: %d, producer: %d, op: try lock %d\n", pid, pid, num);
				sem_wait(&access);
				printf("pid: %d, producer: %d, op: locked\n", pid, pid);
			
				sem_post(&product);
				z++;

				sem_post(&access);
				printf("pid: %d, producer: %d, op: unlock\n", pid, pid);

				sem_post(&full);
			}
			//sleep(64);
		}
	}

	/*
	else if (pid > 3) //consumer
	{
		//printf("OK here\n");
		//int z = 0;
		int num = 0;
		//while (z<4)
		while (1)
		{
			num += 4;

			printf("pid: %d, consumer: %d, op: try lock %d\n", pid, pid, num);

			sem_wait(&access);
			printf("pid: %d, consumer: %d, op: locked\n", pid, pid);

			printf("pid: %d, consumer: %d, op: try consume %d\n", pid, pid, num);
			sem_wait(&product);
			printf("pid: %d, consumer: %d, op: consumed %d\n", pid, pid, num);
			//++z;
			sem_post(&access);
			printf("pid: %d, consumer: %d, op: unlock\n", pid, pid);

			sleep(64);
		}
		//exit();
	}
	*/

	else if (pid > 3) //consumer
	{
		int z;
		int num = 0;
		
		while (1)
		{
			z = 0;
			while (z < 4)
			{
				num++;
				sem_wait(&full);

				printf("pid: %d, consumer: %d, op: try lock %d\n", pid, pid, num);
				sem_wait(&access);
				printf("pid: %d, consumer: %d, op: locked\n", pid, pid);

				printf("pid: %d, consumer: %d, op: try consume %d\n", pid, pid, num);
				sem_wait(&product);
				z++;
				printf("pid: %d, consumer: %d, op: consumed %d\n", pid, pid, num);

				sem_post(&access);
				printf("pid: %d, consumer: %d, op: unlock\n", pid, pid);

				sem_post(&empty);
			}
			//sleep(4);
		}
	}

	exit();
	return 0;
}




//sleep(200);
// ret = fork();  //1
// printf("OK here rettttttttttttttttttttttttttttttttt = %d\n", ret);
// return 0;

/*
	if (ret == 0)  //child
	{
		printf("OK here 11111111111111111111111111111111111111111111111111111111111\n");
		pid = getpid();
		ret = fork(); //2
		printf("OK here rettttttttttttttttttttttttttttttttt = %d\n", ret);

		if (ret == 0)
		{
			printf("OK here 2222222222222222222222222222222222222222222222222222222222222222222\n");

			pid = getpid();
			ret = fork(); //3
			printf("OK here rettttttttttttttttttttttttttttttttt = %d\n", ret);

			if (ret == 0)
			{
				printf("OK here 3333333333333333333333333333333333333333333333333\n");

				pid = getpid();
				ret = fork(); //4

				if (ret == 0)
				{
					printf("OK here 44444444444444444444444444444444444444444\n");

					pid = getpid();
					ret = fork(); //5

					if (ret == 0)
					{
						printf("OK here 5555555555555555555555555555555555555555555\n");

						pid = getpid();
						ret = fork(); //6

						if (ret == 0)
						{
							printf("OK here 66666666666666666666666666666666666666\n");
							pid = getpid();
						}
							
					}
				}
			}
		}
	}
*/
//printf("OK here\n");
