# Kernel module for CS423 in UIUC(MP3)
*Measure major fault, minor fault, and cpu utilization*  

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
  * First `vmalloc (128*PAGE_SIZE)` of virtual memory for latter use (PAGE\_SIZE should be 4KB).  
    The reason of not using `kmalloc` is that 
    (128\*PAGE\_SIZE) is beyond the maximum size that kmalloc can handle (128KB).  
  * Because memory allocated by `vmalloc` is not physically continuous,  
    to prevent the pages being swap out,  
    we have to `SetPageReserved` for page of the every PAGE\_SIZE of memory.  
  * Also because memory allocated by vmalloc is not physically continuous,  
    when user `mmap` a piece of memory,  
    we have to `remap_pfn_range` the virtual memory PAGE\_SIZE by PAGE\_SIZE.  
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
## Analysis  
Case study 1:  
	When user malloc a piece of virtual memory,  
	kernel usually doesn't map the virtual memory to physical memory until user tries to access it,  
	which is the moment that minor page fault happens.  
	If kernel is running out of physical memory, it might swap some piece of memory out to disk.  
	And when user reclaim that piece of memory, major page fault happens.  
	There are two kind of access function in work.  
	One is rand_access, which access the virtual memory between buffer[i] and buffer[i]+1024*1024.  
	Another one is local_access, which actually has nothing to do with the virtual memory, and would never trigger page fault.  
	The total page fault of the first case is much higher than the second case,  
	which makes sense since the second work in the second case has chance to execute local_acess,  
	and other three works have to execute rand_access every time.   
	The drop in two picture is because one of two tasks exit earlier.  
	And the range of drops differ because local_acess doesn't affect the page fault.  

Case study 2:  
	The higher N is, the higher the cpu utilization is.  
	It's because the more tasks we execute,  
	the chance that one of the tasks be context swithed to run is higher,  
	which causing the cpu utilization higher.  
