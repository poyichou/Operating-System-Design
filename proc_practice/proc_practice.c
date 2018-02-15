#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
//#include<linux/linux.h>
#define FILENAME "status"
#define DIRECTORY "mp1"
/*
 struct list_head{
	struct listhead *next;
	struct list_head *prev;
 };
 struct my_cool_list{
	struct list_head list;//kernel's list structure
	int my_cool_data;
	coid* my_cool_void;
 };
 */
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;

static ssize_t mp1_read(struct file *file, char __user *buffer, size_t count, loff_t *data){
	//implement
	return 0;
}

static ssize_t mp1_write(struct file *file, const char __user *buffer, size_t count, loff_t *data){
	//implement
	/******************samaple****************** /
	int copied;
	char*buf;
	buf = (char*) kmalloc(count, GFP_KERNEL);
	copied = 0;
	//put something into the buff, updated copied
	copy_to_user(buffer, buf, copied);
	kfree(buf);
	return copied;
	/ ******************************************/
	return 0;
}
static const struct file_operations mp1_file = {
	.owner = THIS_MODULE,
	.read  = mp1_read,
	.write = mp1_write,
};
int __init mp1_init(void){
	proc_dir = proc_mkdir(DIRECTORY, NULL);
	if(proc_entry == NULL){
		printk(KERN_ALERT "proc_dir create fail\n");
		return -ENOMEM;
	}
	proc_entry = proc_create(FILENAME, 066, proc_dir, &mp1_file);
	if(proc_entry == NULL){
		printk(KERN_ALERT "proc_entry create fail\n");
		return -ENOMEM;
	}
	return 0;
}
void __exit mp1_exit(void)
{
	remove_proc_entry(FILENAME, proc_dir);
	remove_proc_entry(DIRECTORY, NULL);
}
module_init(mp1_init);
module_exit(mp1_exit);
MODULE_LICENSE("GPL");
