#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> 
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1
#define FILENAME "status"
#define DIRECTORY "mp1"
#define MAXSIZE 2048

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static char mesg[MAXSIZE];
static struct list_head HEAD;
static struct timer_list my_timer;

struct pid_time{
	pid_t pid;
	unsigned long time;
	struct list_head list;//kernel's list structure
};

static ssize_t mp1_read(struct file *file, char __user *buffer, size_t count, loff_t *offset){
	//implement
	int result = 0;
	struct pid_time *temp, *tempn;
	char *tmpmsg = kmalloc(MAXSIZE, GFP_KERNEL);
	char *ptr;
	mesg[0] = 0;
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		sprintf(tmpmsg,  "%d: %lu\n", (int)(temp->pid), temp->time);
		if(strlen(mesg) + strlen(tmpmsg) + 1 > MAXSIZE){
			printk(KERN_ALERT "proccess too much\n");
		}else{
			strcat(mesg, tmpmsg);
		}
	}
	kfree(tmpmsg);
	ptr = mesg + *offset;
	while(count > 0 && *ptr != 0)
	{
		put_user(*(ptr++), buffer++);
		count--;
		result++;
	}
	if(result > 0)
		printk(KERN_ALERT "Sent %d characters to the user\n", result);
	*offset += result;
	return result;
}

static ssize_t mp1_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset){
	//implement
	struct pid_time *temp;
	size_t size;
	size = MAXSIZE;
	if(size > count - *offset){
		size = count - *offset;
	}
	if(size > 0){
		if(copy_from_user(mesg, buffer + *offset, size) != 0){
			printk(KERN_ALERT "Failed to get %ld characters from the user\n", size);
			return -EFAULT;
		}
		//printk(KERN_ALERT "Get %s, %ld characters from the user\n", mesg, size);
		*offset += size;
		temp = kmalloc(sizeof(struct pid_time), GFP_KERNEL);
		if(temp == NULL){
			printk(KERN_ALERT "kmaoolc error\n");
			return 0;
		}
		temp->pid = (pid_t)simple_strtol(mesg, NULL, 10);
		if(get_cpu_use((int)(temp->pid), &(temp->time)) != 0){//fail
			printk(KERN_ALERT "get_cpu_use error\n");
			kfree(temp);
			return 0;
		}
		INIT_LIST_HEAD(&(temp->list));
		list_add(&(temp->list), &HEAD);
	}
	return size;
}
static const struct file_operations mp1_file = {
	.owner = THIS_MODULE,
	.read  = mp1_read,
	.write = mp1_write,
};
static void destroy_pid(struct pid_time *del){
	list_del(&(del->list));
	kfree(del);
}
static void update_pid_time(unsigned long data){
	struct pid_time *temp, *tempn;
	printk(KERN_ALERT "update pid time\n");
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		if(get_cpu_use((int)(temp->pid), &(temp->time)) != 0){//proccess not exists anymore
			destroy_pid(temp);
		}
	}
}
// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
	#ifdef DEBUG
   	printk(KERN_ALERT "MP1 MODULE LOADING\n");
   	#endif
	//create proc dir and file
	proc_dir = proc_mkdir(DIRECTORY, NULL);
	if(proc_dir == NULL){
		remove_proc_entry(DIRECTORY, NULL);
		printk(KERN_ALERT "proc_dir create fail\n");
		return -ENOMEM;
	}
	proc_entry = proc_create(FILENAME, 0666, proc_dir, &mp1_file);
	if(proc_entry == NULL){
		remove_proc_entry(FILENAME, proc_dir);
		printk(KERN_ALERT "proc_entry create fail\n");
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&HEAD);// initialize it
	//setup timer
	setup_timer(&my_timer, update_pid_time, 0);
	//setup interval
	if(mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000)) != 0){//jiffies is a global variable
		printk(KERN_ALERT "mod_timer error\n");
	}
	//add_timer(&my_timer);
	//init_timer(&my_timer);
	//my_timer.function = update_pid_time;
	//my_timer.data = ((unsigned long)0);
	//my_timer.expires = jiffies + 50 * HZ;
	printk(KERN_ALERT "MP1 MODULE LOADED\n");
   	return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
	#ifdef DEBUG
   	printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   	#endif
   	// remove proc dir and file
	remove_proc_entry(FILENAME, proc_dir);
	remove_proc_entry(DIRECTORY, NULL);
   
   
	if(del_timer( &my_timer ) != 0){
		printk(KERN_ALERT "del_timer error\n");
	}
	printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
MODULE_LICENSE("GPL");
