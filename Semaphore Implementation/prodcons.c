//Author: William McKibbin
//Date: 2/21/15
//Instructor: Dr. Farnan CS1550 Univ. of Pittsburgh
//This is a simple test program for a kernel semaphore implementation
//The program takes 3 command line arguments, 
//[0] #producers, [1] #consumers, and [2] buffersize
#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
//Semaphore structure with 
struct cs1550_sem
{
	int value;
	//queue back pos and front pos
	struct pqNode* back;
	struct pqNode* front;
};
//buffer header struct
struct bufHeader
{
	int* custPos;
	int* cookPos;
	int pancakes;
	int* endofbuffer; 
};
//wrapper functions for up and down syscalls

void down(struct cs1550_sem* sem) 
{
	syscall(__NR_cs1550_down, sem);
}
void up(struct cs1550_sem* sem) 
{
	syscall(__NR_cs1550_up, sem);
}

int main(int argc, char* argv[])
{
	int i; //trusty iterator
	if(argc != 4)
	{
		printf("Incorrect command line arguments\n");
		return 1;
	}
	//parse comman line args
	int cooks = atoi(argv[1]); //producers
	int customers = atoi(argv[2]);//consumers
	int bufSize = atoi(argv[3]);//buffer size

	//set up shared memory and semaphores
	
	/*buffer works as an array based queue for producing an consuming pancakes
	Producers(cooks) create pancakes at the end of the queue and consumers(customers)
	eat the pancakes at the beginning of queue. When the producers get to the end they
	loop back around to the beginning of the buffer*/
	
	void* buf =  mmap(NULL, sizeof(struct bufHeader) + bufSize, 
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	
	struct bufHeader* kitchen = buf;
	kitchen -> pancakes = 0;
	kitchen -> cookPos = (int*)(kitchen + 1);
	kitchen -> custPos = (int*)(kitchen + 1);
	kitchen -> endofbuffer = (int*)(kitchen + 1) + (bufSize-1);
	//initialize buffer queue
	
	int* initPtr = (int*)(kitchen + 1);
	for(i = 0; i < bufSize; i++)
	{
		*initPtr = 0;
	}
	
	struct cs1550_sem* semaphores = mmap(NULL, sizeof(struct cs1550_sem) * 3, 
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	
	struct cs1550_sem* mutex = semaphores;
	mutex -> value = 1;
	mutex -> back = NULL;
	mutex -> front = NULL;
	struct cs1550_sem* empty = semaphores + 1;
	empty -> value = bufSize;
	empty -> back = NULL;
	empty -> front = NULL;
	struct cs1550_sem* full = semaphores + 2;
	full -> value = 0;
	full -> back = NULL;
	full -> front = NULL;
	
	//create producers
	for(i = 0; i < cooks; i++)
	{
		if(fork() == 0)
		{
			char cookID = i;
			while(1)
			{//produce
				down(empty);
				down(mutex);
				kitchen -> pancakes++;
				printf("Cook %d Produced: Pancake %d\n",i,kitchen -> pancakes);
				sleep(1);
				*kitchen -> cookPos = kitchen -> pancakes;
				if(kitchen -> cookPos == kitchen -> endofbuffer)
				{
					kitchen -> cookPos = (int*)(kitchen + 1);
				}
				else
				{
					kitchen -> cookPos++;
				}
				up(mutex);
				up(full);
			}
		}
	}
	//create consumers
	
	for(i = 0; i < customers; i++)
	{
		if(fork() == 0)
		{	
			
			while(1)
			{
				down(full);
				down(mutex);
				printf("Customer %d Consumed: Pancake %d\n",i,*kitchen -> custPos);
				sleep(1);
				*kitchen -> custPos = 0;
					if(kitchen -> custPos == kitchen -> endofbuffer)
					{
						kitchen -> custPos = (int*)(kitchen + 1);
					}
					else
					{
						kitchen -> custPos++;
					}
				up(mutex);
				up(empty);
			}
		}
	}
	
	int block;
	wait(&block);
	return 0;
}