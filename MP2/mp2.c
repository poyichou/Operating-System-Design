#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> 
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "mp2_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG		1
#define READY		0
#define SLEEPING	1
#define RUNNING		2
#define MAXSIZE		2048
#define DIRECTORY	"mp2"
#define FILENAME	"status"

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static char mesg[MAXSIZE];

static struct list_head HEAD;
static struct kmem_cache *my_cache;//slab cache

static DEFINE_SPINLOCK(sp_lock);
struct mp2_task_struct{
	pid_t			pid;
	unsigned int		task_state;
	unsigned long		task_period;
	unsigned long		task_process_time;
	struct task_struct	*tsk;
	struct timer_list	task_timer;
	struct list_head	list;//kernel's list structure
};
static struct task_struct* kthrd;//kernel thread
static struct mp2_task_struct *currtask = NULL;

static struct mp2_task_struct* highest_priority_task(void){
	struct mp2_task_struct *temp, *tempn;
	struct mp2_task_struct *target = NULL;
	//unsigned long flags;
	unsigned long min_period;
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		//spin_lock_irqsave(&sp_lock, flags);
		if(temp->task_state == READY){
			if(target == NULL || min_period > temp->task_period){
				target = temp;
				min_period = temp->task_period;
			}
		}
		//spin_unlock_irqrestore(&sp_lock, flags);
	}
	return target;
}
static void preempt_task(struct mp2_task_struct *tsk){
	//for old task
	struct sched_param sparam;
	if(tsk != NULL){
		tsk->task_state = READY;
		//set_current_state(TASK_UNINTERRUPTIBLE);//to make it not in running queue after schedule()
		sparam.sched_priority = 0;
		sched_setscheduler(tsk->tsk, SCHED_NORMAL, &sparam);
	}
}
static void set_new_task(struct mp2_task_struct *target){
	struct sched_param sparam;
	//new task
	target->task_state = RUNNING;
	sparam.sched_priority = 99;
	sched_setscheduler(target->tsk, SCHED_FIFO, &sparam);
	currtask = target;
}
static int kthread_fn(void* data){
	struct mp2_task_struct *target;
	unsigned long flags;
	
	while(!kthread_should_stop()){
		spin_lock_irqsave(&sp_lock, flags);
		target = highest_priority_task();
		
		if(target == NULL || (currtask != NULL && target->task_period >= currtask->task_period)){
			//no ready task to be switched
			;
		}else{
			//switch
			preempt_task(currtask);
			set_new_task(target);
			//put it into run queue
			wake_up_process(target->tsk);
		}
		
		spin_unlock_irqrestore(&sp_lock, flags);

		//set kthread
		set_current_state(TASK_INTERRUPTIBLE);
		//sleep
		schedule();
	}
	printk(KERN_ALERT "Thread Stopping\n");
	do_exit(0);
	return 0;
}
static void init_my_kthread(void){
	printk(KERN_ALERT "Creating Thread\n");
	kthrd = kthread_create(kthread_fn, NULL, "mythread");
	wake_up_process(kthrd);
	if(!IS_ERR(kthrd)){
		printk(KERN_ALERT "Thread Created successfully\n");
	}else{
		printk(KERN_ALERT "Thread creation failed\n");
	}
	return;
}
static ssize_t mp2_read(struct file *file, char __user *buffer, size_t count, loff_t *offset){
	//implement
	int result = 0;
	struct mp2_task_struct *temp, *tempn;
	char *tmpmsg = kmalloc(MAXSIZE, GFP_KERNEL);
	char *ptr;
	unsigned long flags;
	spin_lock_irqsave(&sp_lock, flags);
	mesg[0] = 0;//put it after lock, to avoid writen by other 
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		if(temp->task_state == READY){
			sprintf(tmpmsg,  "%d, %lu, %lu, READY\n", (int)(temp->pid), temp->task_process_time, temp->task_period);
		}else if(temp->task_state == RUNNING){
			sprintf(tmpmsg,  "%d, %lu, %lu, RUNNING\n", (int)(temp->pid), temp->task_process_time, temp->task_period);
		}else if(temp->task_state == SLEEPING){
			sprintf(tmpmsg,  "%d, %lu, %lu, SLEEPING\n", (int)(temp->pid), temp->task_process_time, temp->task_period);
		}
		if(strlen(mesg) + strlen(tmpmsg) + 1 > MAXSIZE){
			printk(KERN_ALERT "proccess too much\n");
		}else{
			strcat(mesg, tmpmsg);
		}
	}
	spin_unlock_irqrestore(&sp_lock, flags);
	kfree(tmpmsg);
	//get bytes need to read after last time
	spin_lock_irqsave(&sp_lock, flags);
	ptr = mesg + *offset;
	while(count > 0 && *ptr != 0)
	{
		put_user(*(ptr++), buffer++);
		count--;
		result++;
	}
	if(result > 0)
		printk(KERN_ALERT "Sent %d characters to the user\n", result);
	//record mesg read this time
	*offset += result;
	spin_unlock_irqrestore(&sp_lock, flags);
	return result;
}

static void init_slab_cache(void){
	my_cache = kmem_cache_create(
			"my_cache",
			sizeof(struct mp2_task_struct),
			0,
			SLAB_HWCACHE_ALIGN,
			NULL);
	return;
}
static void destroy_slab_cache(void){
	if(my_cache) kmem_cache_destroy(my_cache);
	return;
}

static int add_task(struct mp2_task_struct *temp){
	//add a pid
	unsigned long flags;
	spin_lock_irqsave(&sp_lock, flags);
	INIT_LIST_HEAD(&(temp->list));
	
	list_add(&(temp->list), &HEAD);
	
	spin_unlock_irqrestore(&sp_lock, flags);
	return 0;
}
static int admit_control(unsigned long period, unsigned long computation){
	struct mp2_task_struct *temp, *tempn;
	unsigned long flags;
	unsigned long P = 0, C = 0;
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		spin_lock_irqsave(&sp_lock, flags);
		P += temp->task_period;
		C += temp->task_process_time;
		spin_unlock_irqrestore(&sp_lock, flags);
	}
	P += period;
	C += computation;
	if((C * 1000 / P) <= 693){
		return 0;
	}
	return -1;
}
static void timer_handler(unsigned long data){
	struct mp2_task_struct *object = (struct mp2_task_struct*)data;
	unsigned long flags;
	//printk(KERN_ALERT "timer_handler\n");
	spin_lock_irqsave(&sp_lock, flags);
	//make timer periodic
	if(mod_timer(&(object->task_timer), jiffies + msecs_to_jiffies(object->task_period)) != 0){//jiffies is a global variable
		printk(KERN_ALERT "mod_timer error\n");
	}
	if(object->task_state == SLEEPING){
		object->task_state = READY;
	}
	spin_unlock_irqrestore(&sp_lock, flags);
	//trigger kernel thread
	wake_up_process(kthrd);
}
static void registration(int pid, unsigned long period, unsigned long computation){
	//For REGISTRATION: “R, PID, PERIOD, COMPUTATION”
	struct mp2_task_struct *object;
	//unsigned long flags;
	if(admit_control(period, computation) == -1){
		return;
	}
	//slab alloc
	object = kmem_cache_alloc(my_cache, GFP_KERNEL);
	if (!object) {
		//kmem_cache_free(my_cache, object);
		printk(KERN_ALERT "kmem_cache_alloc error\n");
		return;
	}
	//spin_lock_irqsave(&sp_lock, flags);
	object->tsk = find_task_by_pid((unsigned int)pid);
	//spin_unlock_irqrestore(&sp_lock, flags);
	object->pid = (pid_t)pid;
	object->task_state = SLEEPING;
	object->task_period = period;
	object->task_process_time = computation;
	
	//setup timer
	setup_timer(&(object->task_timer), timer_handler, (unsigned long)object);
	//setup interval
	if(mod_timer(&(object->task_timer), jiffies + msecs_to_jiffies(period)) != 0){//jiffies is a global variable
		printk(KERN_ALERT "mod_timer error\n");
	}
	add_task(object);
}

//free node
static void __destroy_pid(struct mp2_task_struct *del){
	if(del == currtask){
		currtask = NULL;
	}
	//remove timer 
	del_timer(&(del->task_timer));
	list_del(&(del->list));
	kmem_cache_free(my_cache, del);
}
//free all node
static void destroy_all_pid(void){
	struct mp2_task_struct *temp, *tempn;
	unsigned long flags;
	spin_lock_irqsave(&sp_lock, flags);
	
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		__destroy_pid(temp);
	}
	
	spin_unlock_irqrestore(&sp_lock, flags);
}

static void deregistration(pid_t pid){
	struct mp2_task_struct *temp, *tempn;
	unsigned long flags;
	spin_lock_irqsave(&sp_lock, flags);
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		
		if(temp->pid == pid){
			//free node
			__destroy_pid(temp);
			
			spin_unlock_irqrestore(&sp_lock, flags);
			return;
		}
		
	}
	spin_unlock_irqrestore(&sp_lock, flags);
}
static void set_task_state_sleep(pid_t pid){
	struct mp2_task_struct *temp, *tempn;
	unsigned long flags;
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		spin_lock_irqsave(&sp_lock, flags);
		if(temp->pid == pid){
			temp->task_state = SLEEPING;
			set_task_state(temp->tsk, TASK_UNINTERRUPTIBLE);
			currtask = NULL;
			
			spin_unlock_irqrestore(&sp_lock, flags);
			return;
		}
		spin_unlock_irqrestore(&sp_lock, flags);
	}
	printk(KERN_ALERT "yield nothing\n");
}
static void my_yield(pid_t pid){
	set_task_state_sleep(pid);
	//set_current_state(TASK_UNINTERRUPTIBLE);//to make it not in running queue after schedule()
	//trigger kernel thread
	wake_up_process(kthrd);
	//sleep
	schedule();
}
static char* my_strtok(char*str){
	int i = 0;
	for(i = 0 ; i < strlen(str) ; i++){
		if(str[i] == ' '){
			str[i] = 0;
			return str + i + 1;
		}
	}
	return NULL;
}
static ssize_t mp2_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset){
	//implement
	size_t size;
	char *pidstr, *periodstr, *compstr;
	char *tmpmsg = kmalloc(MAXSIZE, GFP_KERNEL);
	unsigned long flags;
	size = MAXSIZE;
	//For REGISTRATION: “R PID PERIOD COMPUTATION”
	//For YIELD: “Y PID”
	//For DE-REGISTRATION: “D PID”
	//get bytes need to write after last time
	if(size > count - *offset){
		size = count - *offset;
	}
	if(size > 0){
		spin_lock_irqsave(&sp_lock, flags);
		memset(mesg, 0, MAXSIZE);
		if(copy_from_user(mesg, buffer + *offset, size) != 0){
			printk(KERN_ALERT "Failed to get %ld characters from the user\n", size);
			return -EFAULT;
		}
		strcpy(tmpmsg, mesg);
		spin_unlock_irqrestore(&sp_lock, flags);
		//record mesg writen this time
		*offset += size;
		printk(KERN_ALERT "%s\n", tmpmsg);
		if(tmpmsg[0] == 'R'){
			pidstr = my_strtok(tmpmsg);
			periodstr = my_strtok(pidstr);
			compstr = my_strtok(periodstr);
			
			registration(	(pid_t)simple_strtol(pidstr, NULL, 10), 
					(unsigned long)simple_strtol(periodstr, NULL, 10),
					(unsigned long)simple_strtol(compstr, NULL, 10));

		}else if(tmpmsg[0] == 'Y'){
			pidstr = my_strtok(tmpmsg);
			my_yield((pid_t)simple_strtol(pidstr, NULL, 10));
		}else if(tmpmsg[0] == 'D'){
			pidstr = my_strtok(tmpmsg);
			deregistration((pid_t)simple_strtol(pidstr, NULL, 10));
		}
	}
	kfree(tmpmsg);
	return size;
}
static const struct file_operations mp2_file = {
	.owner = THIS_MODULE,
	.read  = mp2_read,
	.write = mp2_write,
};
// mp2_init - Called when module is loaded
int __init mp2_init(void)
{
	#ifdef DEBUG
   	printk(KERN_ALERT "MP2 MODULE LOADING\n");
   	#endif
	//create proc dir and file
	proc_dir = proc_mkdir(DIRECTORY, NULL);
	if(proc_dir == NULL){
		remove_proc_entry(DIRECTORY, NULL);
		printk(KERN_ALERT "proc_dir create fail\n");
		return -ENOMEM;
	}
	proc_entry = proc_create(FILENAME, 0666, proc_dir, &mp2_file);
	if(proc_entry == NULL){
		remove_proc_entry(FILENAME, proc_dir);
		printk(KERN_ALERT "proc_entry create fail\n");
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&HEAD);// initialize it
	init_my_kthread();
	init_slab_cache();
	printk(KERN_ALERT "MP2 MODULE LOADED\n");
   	return 0;   
}

// mp2_exit - Called when module is unloaded
void __exit mp2_exit(void)
{
	#ifdef DEBUG
   	printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
   	#endif
   	// remove proc dir and file
	remove_proc_entry(FILENAME, proc_dir);
	remove_proc_entry(DIRECTORY, NULL);
   
	destroy_slab_cache();
	//in case of unloaded with some registered processes are still running
	destroy_all_pid();
	
	if (kthrd){
		kthread_stop(kthrd);
		printk(KERN_ALERT "Kernel thread stopped");
	}
	printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);
MODULE_LICENSE("GPL");
