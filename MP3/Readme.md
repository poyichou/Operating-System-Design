# Kernel module for CS423 in UIUC(MP3)
  
## Implementation:  
* create a proc file to handle registration and unregistration  
  * R \<PID\> for registration -> store it in the list.  
    If it's the first one in the list  
    -> create a delayed work queue and assign a delayed work with interval of 50ms  
    In other word, the delayed work would be executed 20 time in a second.
  * U \<PID\> for unregistration  
    If there's is no task in the list  
    -> cancel the existing delayed work  
    -> flush the delayed work queue  
    -> destroy the delayed work queue  
* create a character device to handle mmap in userspace  
  * First `vmalloc (128\*PAGE\_SIZE)` of virtual memory for latter use(PAGE\_SIZE should be 4KB).  
    The reason of not using `kmalloc` is that 
    (128\*PAGE\_SIZE) is beyond the maximum size that kmalloc can handle (128KB).  
  * Because memory allocated by `vmalloc` is not physically continuous,  
    to prevent the pages being swap out,  
    we have to `SetPageReserved` for page of the every PAGE\_SIZE of memory.  
  * Also because memory allocated by vmalloc is not physically continuous,  
    when user `mmap` a piece of memory,  
    we have to `remap_pfn_range` PAGE\_SIZE by PAGE\_SIZE the virtual memory.  
* use `struct list_head` to store information of task  
  The list store pid and the accummulated value of utime and stime of every task  
  and update the accummulated value when the delayed work executed.  
* sample with delayed work queue  
  store jiffies, minor page fault, major page fault and cpu utilization as four `unsigned long` in the virtual memory  

## Testing
* How to compile:  
	`make`  
* After compiling, load the module:  
	`make install`  
* And create the node:  
	`make node`  
* Then, for case study 1:  
	`./case1.sh 1` and `./case1.sh 2`  
  You would get profile1.data and profile2.data respectively  
* For case study 2:  
	`./case2.sh 1` and `./case2.sh 5` `./case2.sh 11`  
  It would run 1, 5, and 11 instances of work as document says.  
  And you would get profile3.data, profile4.data and profile5.data as result respectively  
* To unload the module:  
	`make uninstall`  
