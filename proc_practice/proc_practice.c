#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> 
//#include<linux/list.h>
#define FILENAME "status"
#define DIRECTORY "mp1"
#define MAXSIZE 2048
/*
 struct list_head{
	struct listhead *next;
	struct list_head *prev;
 };
 struct my_cool_list{
	struct list_head list;//kernel's list structure
	int my_cool_data;
	void* my_cool_void;
 };
 */
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static char mesg[MAXSIZE];

static ssize_t mp1_read(struct file *file, char __user *buffer, size_t count, loff_t *offset){
	//implement
	int result = 0;
	
	char *hello = "hello\n";
	hello += *offset;
	while(count > 0 && *hello != 0){
		put_user(*(hello++), buffer++);
		count--;
		result++;
	}
	printk(KERN_ALERT "Sent %d characters to the user\n", result);
	*offset += result;
	return result;
}

static ssize_t mp1_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset){
	//implement
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
		printk(KERN_ALERT "Get %s, %ld characters from the user\n", mesg, size);
		*offset += size;
	}
	return size;
}
static const struct file_operations mp1_file = {
	.owner = THIS_MODULE,
	.read  = mp1_read,
	.write = mp1_write,
};
int __init mp1_init(void){
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
	printk(KERN_ALERT "proc_practice installed\n");
	return 0;
}
void __exit mp1_exit(void)
{
	remove_proc_entry(FILENAME, proc_dir);
	remove_proc_entry(DIRECTORY, NULL);
	printk(KERN_ALERT "proc_practice uninstalled\n");
}
module_init(mp1_init);
module_exit(mp1_exit);
MODULE_LICENSE("GPL");
