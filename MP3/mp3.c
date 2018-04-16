#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> 
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/mm.h> /* vmalloc_to_pfn */
#include <linux/vmalloc.h>/* vmalloc */

#include "mp3_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("poyiyc2");
MODULE_DESCRIPTION("CS-423 MP3");

#define DEBUG
#define MAXSIZE		2048
#define PROC_DIRECTORY	"mp3"
#define PROC_FILENAME	"status"
#define CHRDEV_NAME	"mp3_chrdev"
#define ALLOC_SIZE	(128*PAGE_SIZE)
#define SAMPLE_LEN	(4*sizeof(unsigned long)*12000)

static void work_handler(struct work_struct *data);

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static char mesg[MAXSIZE];
static int task_count = 0;
static void* vmalloc_addr = NULL;
static void* curr = NULL;
static int major = 0;
static unsigned long prev_jiffies = 0;
/* previous stime + utime */
static unsigned long prev_su_time = 0;

static struct list_head HEAD;
static DEFINE_SPINLOCK(sp_lock);
static DECLARE_DELAYED_WORK(dwork, work_handler);
static struct workqueue_struct *workqueue;

struct pid_list{
	pid_t			pid;
	/* kernel's list structure */
	struct list_head	list;
};
/*
 * If a function name start with "__", there would be at least one spin_lock 
 * in it.
 */
static void destroy_pid(struct pid_list *del){
	list_del(&(del->list));
	kfree(del);
	task_count--;
	if(task_count == 0){
		flush_workqueue(workqueue);
		destroy_workqueue(workqueue);
	}
}
/*
 * CPU utilization = cpu_time / wall_time = ( stime + utime ) / time.
 * Major page fault is a fault that is handled by using a disk I/O operation 
 * (e.g., memory-mapped file).
 * Minor page fault is a fault that is handled without using a disk I/O 
 * operation (e.g., allocated by the malloc() function).
 */
static void store_sample(unsigned long accm_min_flt, unsigned long accm_maj_flt, unsigned long accm_sutime)
{
	if (curr == NULL || curr + (sizeof(unsigned long) * 4) > vmalloc_addr + SAMPLE_LEN) {
		curr = vmalloc_addr;
	}
	*(((unsigned long *)curr) + 0) = jiffies;
	*(((unsigned long *)curr) + 1) = accm_min_flt;
	*(((unsigned long *)curr) + 2) = accm_maj_flt;
	*(((unsigned long *)curr) + 3) = (accm_sutime - prev_su_time) / (jiffies - prev_jiffies);
	prev_jiffies = jiffies;
	prev_su_time = accm_sutime;
	curr += (sizeof(unsigned long) * 4);
}
static void scan_cpu_use(unsigned long *accm_min_flt, unsigned long *accm_maj_flt, unsigned long *accm_sutime)
{
	struct pid_list *temp, *tempn;
	struct task_struct *task;
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		task = find_task_by_pid(temp->pid);
		if(task == NULL){
			/* proccess not exists anymore */
			destroy_pid(temp);
		} else {
			*accm_min_flt   += task->min_flt;
			*accm_maj_flt   += task->maj_flt;
			*accm_sutime    += task->utime + task->stime;
		}
	}
}
/**
 * work_handler() is use to sample and store it in vmalloc_addr (as 
 * circular array).
 * each sample consist of four unsigned long: jiffies, min_flt, maj_flt, cpu_utilization
 */
static void work_handler(struct work_struct *data)
{
	unsigned long accm_min_flt = 0, accm_maj_flt = 0, accm_sutime = 0;
	unsigned long flags;
	queue_delayed_work(workqueue, &dwork, msecs_to_jiffies(50));

	spin_lock_irqsave(&sp_lock, flags);
	scan_cpu_use(&accm_min_flt, &accm_maj_flt, &accm_sutime);
	if(prev_jiffies == 0){
		/* test code, but the statement shouldn't be true */
		prev_jiffies = jiffies;
		prev_su_time = accm_sutime;
		spin_unlock_irqrestore(&sp_lock, flags);
		printk(KERN_ALERT "work_handler logically wrong\n");
		return;
	}
	store_sample(accm_min_flt, accm_maj_flt, accm_sutime);

	spin_unlock_irqrestore(&sp_lock, flags);
}
static char *my_strtok(char *str){
	int i = 0;
	for(i = 0 ; i < strlen(str) ; i++){
		if(str[i] == ' '){
			str[i] = 0;
			return str + i + 1;
		}
	}
	return NULL;
}
static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *offset){
	int result = 0;
	struct pid_list *temp, *tempn;
	char *tmpmsg = kmalloc(MAXSIZE, GFP_KERNEL);
	char *ptr;
	unsigned long flags;

	spin_lock_irqsave(&sp_lock, flags);
	mesg[0] = 0;//put it after lock, to avoid writen by other 
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		sprintf(tmpmsg,  "%d\n", (int)(temp->pid));
		if(strlen(mesg) + strlen(tmpmsg) + 1 > MAXSIZE){
			printk(KERN_ALERT "proccess too much\n");
		}else{
			strcat(mesg, tmpmsg);
		}
	}
	//get bytes need to read after last time
	ptr = mesg + *offset;
	while(count > 0 && *ptr != 0)
	{
		put_user(*(ptr++), buffer++);
		count--;
		result++;
	}
	spin_unlock_irqrestore(&sp_lock, flags);

	if(result > 0)
		printk(KERN_ALERT "Sent %d characters to the user\n", result);
	//record mesg read this time
	*offset += result;
	kfree(tmpmsg);
	return result;
}
static int __add_task(struct pid_list *tsk){
	unsigned long flags;
	spin_lock_irqsave(&sp_lock, flags);
	INIT_LIST_HEAD(&(tsk->list));
	list_add(&(tsk->list), &HEAD);
	task_count++;
	if(task_count == 1){
		workqueue = create_workqueue("mp3_wq");
		prev_jiffies = jiffies;
		prev_su_time = 0;
		queue_delayed_work(workqueue, &dwork, msecs_to_jiffies(50));
	}
	spin_unlock_irqrestore(&sp_lock, flags);
	return 0;
}
static void __registration(pid_t pid){
	struct pid_list *object = kmalloc(sizeof(struct pid_list), GFP_KERNEL);
	object->pid = pid;
	if(find_task_by_pid((unsigned int)pid) == NULL){
		printk(KERN_ALERT "find_task_by_pid failed\n");
		kfree(object);
		return;
	}
	__add_task(object);
}
static void __unregistration(pid_t pid){
	unsigned long flags;
	struct pid_list *temp, *tempn;
	spin_lock_irqsave(&sp_lock, flags);
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		if(temp->pid == pid){
			destroy_pid(temp);
			spin_unlock_irqrestore(&sp_lock, flags);
			return;
		}
	}
	spin_unlock_irqrestore(&sp_lock, flags);
}
/**
 * For REGISTRATION: "R PID"
 * For UNREGISTRATION: "U PID"
 */
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset){
	char *pidstr;
	char *tmpmsg;
	size_t size;
	unsigned long flags;
	size = MAXSIZE;
	/* get bytes need to write after last time */
	if(size > count - *offset){
		size = count - *offset;
	}
	if(size > 0){
		tmpmsg = kmalloc(MAXSIZE, GFP_KERNEL);
		spin_lock_irqsave(&sp_lock, flags);
		/* memset(mesg, 0, MAXSIZE); */
		if(copy_from_user(mesg, buffer + *offset, size) != 0){
			printk(KERN_ALERT "Failed to get %ld characters from the user\n", size);
			return -EFAULT;
		}
		strcpy(tmpmsg, mesg);
		if(tmpmsg[strlen(tmpmsg) - 1] == '\n') {
			tmpmsg[strlen(tmpmsg) - 1] = 0;
		}
		spin_unlock_irqrestore(&sp_lock, flags);
		//record mesg writen this time
		*offset += size;
		printk(KERN_ALERT "write to proc file: [%s]\n", tmpmsg);
		if(tmpmsg[0] == 'R'){
			pidstr = my_strtok(tmpmsg);
			__registration((pid_t)simple_strtol(pidstr, NULL, 10));
		}else if(tmpmsg[0] == 'U'){
			pidstr = my_strtok(tmpmsg);
			__unregistration((pid_t)simple_strtol(pidstr, NULL, 10));
		}
		kfree(tmpmsg);
	}
	return size;
}

/* circularly assign addr. */
static void *vmalloc_addr_add(void *addr, unsigned long i){
	if (addr + i > vmalloc_addr + ALLOC_SIZE - PAGE_SIZE) {
		return vmalloc_addr;
	} else {
		return addr + i;
	}
}

static int device_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int device_mmap(struct file *filp, struct vm_area_struct *vma) 
{
	unsigned long i;
	unsigned long pfn;
	unsigned long len = vma->vm_end - vma->vm_start;
	printk(KERN_ALERT "remap_pfn_rang len=%lu\n", len);
	for (i = 0; i < len; i += PAGE_SIZE) {
		pfn = vmalloc_to_pfn(vmalloc_addr_add(vmalloc_addr, i));
		if (pfn == -EINVAL) {
			printk(KERN_ALERT "vmalloc_to_pfn vmalloc_addr:[%lu] failed.\n",
					(unsigned long)(vmalloc_addr_add(vmalloc_addr, i)));
			return -1;
		}
		/* vma->vm_page_prot should be PAGE_SHARED */
		if (remap_pfn_range(vma, vma->vm_start + i, pfn, PAGE_SIZE, 
					vma->vm_page_prot) != 0) {
			printk(KERN_ALERT "remap_pfn_rang page:[%lu] failed.\n", pfn);
			return -1;
		}
	}
	printk(KERN_ALERT "remap_pfn_rang all page ok.\n");
	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}
static const struct file_operations chrdev_file = {
	.owner   = THIS_MODULE,
	.open    = device_open,		/* empty */
	.mmap    = device_mmap,		/* matters!! */
	.release = device_release,	/* empty */
};
static const struct file_operations proc_file = {
	.owner = THIS_MODULE,
	.read  = proc_read,
	.write = proc_write,
};
/* called when installing the module */
static void __vmalloc_set_reserved(void)
{
	int i;
	unsigned long flags;
	struct page *ppage;
	vmalloc_addr = vmalloc(ALLOC_SIZE);
	spin_lock_irqsave(&sp_lock, flags);
	for (i = 0; i < ALLOC_SIZE; i += PAGE_SIZE) {
		ppage = vmalloc_to_page((void *)(vmalloc_addr + i));
		/* define in linux/page-flags.h */
		SetPageReserved(ppage); /* 2.6.0~2.6.18 */
		/* ppage->vm_flag = VM_RESERVED; *//* 2.6.25~ */
	}
	/* initialization according to the implementation of monitor.c */
	for (i = 0; i < sizeof(unsigned long) * 4 * 600 * 20; i += sizeof(unsigned long)) {
		*((unsigned long *)(vmalloc_addr + i)) = (unsigned long)(-1);
	}
	spin_unlock_irqrestore(&sp_lock, flags);

}
static void __vmalloc_clear(void)
{
	int i;
	unsigned long flags;
	struct page *ppage;
	spin_lock_irqsave(&sp_lock, flags);
	for (i = 0; i < ALLOC_SIZE; i += PAGE_SIZE) {
		ppage = vmalloc_to_page((void *)(vmalloc_addr + i));
		/* define in linux/page-flags.h */
		ClearPageReserved(ppage); /* 2.6.0~2.6.18 */
		/* ppage->vm_flag = VM_RESERVED; *//* 2.6.25~ */
	}
	spin_unlock_irqrestore(&sp_lock, flags);
	vfree(vmalloc_addr);
}
/* called when uninstalling the module */
static void __clear_work_queue(void)
{
	unsigned long flags;
	spin_lock_irqsave(&sp_lock, flags);

	if(task_count > 0){
		flush_workqueue(workqueue);
		destroy_workqueue(workqueue);
	}
	spin_unlock_irqrestore(&sp_lock, flags);
}
static void __destroy_all_pid(void)
{
	unsigned long flags;
	struct pid_list *temp, *tempn;
	spin_lock_irqsave(&sp_lock, flags);
	
	list_for_each_entry_safe(temp, tempn, &HEAD, list) {
		destroy_pid(temp);
	}
	
	spin_unlock_irqrestore(&sp_lock, flags);
}
int __init mp3_init(void)
{
	#ifdef DEBUG
   	printk(KERN_ALERT "MP3 MODULE LOADING\n");
   	#endif
	//create proc dir and file
	proc_dir = proc_mkdir(PROC_DIRECTORY, NULL);
	if(proc_dir == NULL){
		remove_proc_entry(PROC_DIRECTORY, NULL);
		printk(KERN_ALERT "proc_dir create fail\n");
		return -ENOMEM;
	}
	proc_entry = proc_create(PROC_FILENAME, 0666, proc_dir, &proc_file);
	if(proc_entry == NULL){
		remove_proc_entry(PROC_FILENAME, proc_dir);
		printk(KERN_ALERT "proc_entry create fail\n");
		return -ENOMEM;
	}
	/* register character device */
	major = register_chrdev(0, CHRDEV_NAME, &chrdev_file);
	__vmalloc_set_reserved();
	INIT_LIST_HEAD(&HEAD);
	#ifdef DEBUG
	printk(KERN_ALERT "MP3 MODULE LOADED\n");
   	#endif
   	return 0;   
}
void __exit mp3_exit(void)
{
	#ifdef DEBUG
   	printk(KERN_ALERT "MP3 MODULE UNLOADING\n");
   	#endif
   	// remove proc dir and file
	remove_proc_entry(PROC_FILENAME, proc_dir);
	remove_proc_entry(PROC_DIRECTORY, NULL);
   
	//in case our work in workqueue not executed
	__clear_work_queue();

	//in case of unloaded with some registered processes are still running
	__destroy_all_pid();
	//unregister character device
	unregister_chrdev(major, CHRDEV_NAME);
	__vmalloc_clear();
	
	#ifdef DEBUG
	printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
   	#endif
}
module_init(mp3_init);
module_exit(mp3_exit);
