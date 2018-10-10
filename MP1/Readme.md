# Kernel module for CS423 in UIUC  
*Measure running time of registered processes*  
How to compile:  
	`make`  
After compiling, load the module:  
	if you are root:  
	`insmod mp1.ko`  
	if you are not:  
	`sudo insmod mp1.ko`  
  
Then, we could register a process by running:  
	`./userapp &`  
we could register many processes, for example,  
	`./userapp & ./userapp &`  
each userapp would print the result before it exits  
  
To unload the module:  
	if you are root:  
	`rmmod mp1`  
	if you are not:  
	`sudo rmmod mp1`  
