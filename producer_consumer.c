#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/tty.h>
#include <linux/uidgid.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/time_namespace.h>
#include <linux/time.h>

#include <linux/proc_fs.h>
#include <linux/slab.h>

#define MAX_BUFFER_SIZE 500
#define MAX_NO_OF_PRODUCERS 1
#define MAX_NO_OF_CONSUMERS 100

#define PCINFO(s, ...) pr_info("###[%s]###" s, __FUNCTION__, ##__VA_ARGS__)

unsigned long long total_time_elapsed = 0;

// use this struct to store the process information
struct process_info
{
	unsigned long pid;
	unsigned long long start_time;
	unsigned long long boot_time;
} process_default_info = {0, 0};

int total_no_of_process_produced = 0;
int total_no_of_process_consumed = 0;

int end_flag = 0;

char producers[MAX_NO_OF_PRODUCERS][12] = {"kProducer-X"};
char consumers[MAX_NO_OF_CONSUMERS][12] = {"kConsumer-X"};

static struct task_struct *ctx_producer_thread[MAX_NO_OF_PRODUCERS];
static struct task_struct *ctx_consumer_thread[MAX_NO_OF_CONSUMERS];

// use fill and use to keep track of the buffer
struct process_info buffer[MAX_BUFFER_SIZE];
int fill = 0;
int use = 0;

static int buffSize, prod, cons, uuid;
module_param(uuid, int, 0644);
module_param(buffSize, int, 0644);
module_param(prod, int, 0644);
module_param(cons, int, 0644);

struct semaphore empty;
struct semaphore full;
struct semaphore mutexLock;

int producer_thread_function(void *pv)
{	
	PCINFO("[%s] kthread Producer Created Successfully\n", current->comm);
	
	allow_signal(SIGKILL);
	struct task_struct *task;

	for_each_process(task)
	{
		if (task->cred->uid.val == uuid)
		{
			// Wait for a slot in the buffer to be empty
			down(&empty);

			// Aquire Mutex lock to enter critical section
			down(&mutexLock);

			if(kthread_should_stop())
				break;

			// Perform the shared Memory Operations
			buffer[fill].pid = task->pid;
			buffer[fill].start_time = task->start_time;
			buffer[fill].boot_time = task->start_boottime;
			fill = (fill + 1) % buffSize;

			total_no_of_process_produced++;
			PCINFO("[%s] Produce-Item#:%d at buffer index: %d for PID:%d \n", current->comm,
				total_no_of_process_produced, (fill + buffSize - 1) % buffSize, task->pid);
			
			// Release the Mutex lock
			up(&mutexLock);
			
			// Signal that a slot in the buffer has been filled
			up(&full);

		}
		
	}

	PCINFO("[%s] Producer Thread stopped.\n", current->comm);
	ctx_producer_thread[0] = NULL;
	return 0;
}

int consumer_thread_function(void *pv)
{
	PCINFO("[%s] kthread Consumer Created Successfully\n", current->comm);
	
	allow_signal(SIGKILL);
	int no_of_process_consumed = 0;

	while (!kthread_should_stop())
	{
		// check the full semaphor to determine if there is a buffer slot to consume
		down(&full);
		
		// take the MUTEX lock to pause all other threads
		down(&mutexLock);

		if (end_flag == 1)
			break;

		// Perform the shared Memory Operation
		// Storing the buffer data for local operation
		unsigned long process_pid = buffer[use].pid;
		unsigned long long start_time_ns = buffer[use].start_time;

		// Operating on the data of the buffer and incrementing
		buffer[use] = process_default_info;
		use = (use + 1) % buffSize;

		// Release the MUTEX lock to wake up a sleeping thread
		up(&mutexLock);

		// increment the empty semaphor to signal the producer that there is a new empty slot
		up(&empty);

		unsigned long long ktime = ktime_get_ns();
		unsigned long long process_time_elapsed = (ktime - start_time_ns) / 1000000000;
		total_time_elapsed += ktime - start_time_ns;

		unsigned long long process_time_hr = process_time_elapsed / 3600;
		unsigned long long process_time_min = (process_time_elapsed - 3600 * process_time_hr) / 60;
		unsigned long long process_time_sec = (process_time_elapsed - 3600 * process_time_hr) - (process_time_min * 60);

		no_of_process_consumed++;
		total_no_of_process_consumed++;
		PCINFO("[%s] Consumed Item#-%d on buffer index:%d::PID:%lu \t Elapsed Time %llu:%llu:%llu \n", current->comm,
			   no_of_process_consumed, (use + buffSize - 1) % buffSize, process_pid, process_time_hr, process_time_min, process_time_sec);
	}

	PCINFO("[%s] Consumer Thread stopped.\n", current->comm);
	return 0;
}

char *replace_char(char *str, char find, char replace)
{
	char *current_pos = strchr(str, find);
	while (current_pos)
	{
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
	return str;
}

void name_threads(void)
{
	for (int index = 0; index < prod; index++)
	{
		char id = (index + 1) + '0';
		strcpy(producers[index], "kProducer-X");
		strcpy(producers[index], replace_char(producers[index], 'X', id));
	}

	for (int index = 0; index < cons; index++)
	{
		char id = (index + 1) + '0';
		strcpy(consumers[index], "kConsumer-X");
		strcpy(consumers[index], replace_char(consumers[index], 'X', id));
	}
}

static int __init thread_init_module(void)
{
	PCINFO("CSE330 Project-3 Kernel Module Inserted\n");
	PCINFO("Kernel module received the following inputs: UID:%d, Buffer-Size:%d, No of Producer:%d, No of Consumer:%d", uuid, buffSize, prod, cons);

	if (buffSize > 0 && (prod >= 0 && prod < 2))
	{
		sema_init(&empty, buffSize);
		sema_init(&full, 0);
		sema_init(&mutexLock, 1);

		name_threads();

		//Initializes the buffer with process info with attributes: PID, Start time, Boot time
		for (int index = 0; index < buffSize; index++)
			buffer[index] = process_default_info;

		// Create Producer threads
		for (int i = 0; i < prod; i++) {
			ctx_producer_thread[i] = kthread_run(producer_thread_function, NULL, producers[i]);
		}

		// Create Consumer Threads
		for (int i = 0; i < cons; i++) {
			ctx_consumer_thread[i] = kthread_run(consumer_thread_function, NULL, consumers[i]);
		}
	}
	else
	{
		// Input Validation Failed
		PCINFO("Incorrect Input Parameter Configuration Received. No kernel threads started. Please check input parameters.");
		PCINFO("The kernel module expects buffer size (a positive number) and # of producers(0 or 1) and # of consumers > 0");
	}

	return 0;
}

static void __exit thread_exit_module(void)
{
	if (buffSize > 0)
	{
		while (1)
		{
			// consumed should equal produced
			// We will have haning processes if consumers == 0 or producers == 0
			if (total_no_of_process_consumed == total_no_of_process_produced || !cons || !prod)
			{
				if (!cons)
				{
					up(&empty);
				}

				for (int index = 0; index < prod; index++)
				{
					if (ctx_producer_thread[index])
					{
						kthread_stop(ctx_producer_thread[index]);
					}
				}

				end_flag = 1;

				for (int index = 0; index < cons; index++)
				{
					up(&full);
					up(&mutexLock);
				}

				for (int index = 0; index < cons; index++)
				{
					if (ctx_consumer_thread[index]){
						kthread_stop(ctx_consumer_thread[index]);
					}
				}
				break;
			}
			else
				continue;
		}

		// total_time_elapsed is now in nsec
		total_time_elapsed = total_time_elapsed / 1000000000;

		unsigned long long total_time_hr = total_time_elapsed / 3600;
		unsigned long long total_time_min = (total_time_elapsed - 3600 * total_time_hr) / 60;
		unsigned long long total_time_sec = (total_time_elapsed - 3600 * total_time_hr) - (total_time_min * 60);

		PCINFO("Total number of items produced: %d", total_no_of_process_produced);
		PCINFO("Total number of items consumed: %d", total_no_of_process_consumed);
		PCINFO("The total elapsed time of all processes for UID %d is \t%llu:%llu:%llu  \n", uuid, total_time_hr, total_time_min, total_time_sec);
	}

	PCINFO("CSE330 Project 3 Kernel Module Removed\n");
}

module_init(thread_init_module);
module_exit(thread_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicholas Lambiase & Conor Yosick");
MODULE_DESCRIPTION("CSE330 2024 Spring Project 3 Process Management\n");
MODULE_VERSION("0.1");
