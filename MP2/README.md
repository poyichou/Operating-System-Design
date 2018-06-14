# Kernel module for CS423 in UIUC  
*simple scheduler for registered processes*  

*	Implementation:  
	change task policy with sched_setscheduler()  
	make kernel thread sleep after one context switch with set_current_state(TASK_INTERRUPTIBLE) and schedule()  
*	Design decision:  
	assign timmer to each task which simply expires every period and set the task to READY if it's state is SLEEPING  
	timer would trigger dispatching thread, which check if context switch is needed (only check tasks that are in READY state)  
*	Test:  
	*	How to compile:  
			`make`  
	*	After compiling, load the module:  
			if you are root : `insmod mp2.ko`  
			if you are not  : `sudo insmod mp2.ko`  
	  
	*	Then, test with one task by running:  
			`./userapp &`  
			or  
			`./userapp [period] [computation] [jobs]`  
			if period and computation not specified, they are set to 600, 100 and 5  
		we could test with many tasks by running command above several times or just run  
			`./test.sh`  
			it test 3 tasks with different periods and computations each for 5 times  
	  
	*	To unload the module:  
			if you are root :  
			`rmmod mp2`  
			if you are not  :  
			`sudo rmmod mp2` 
