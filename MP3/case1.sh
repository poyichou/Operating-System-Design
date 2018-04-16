#!/bin/bash
#usage: [program] [num]
num=$1  
if [ $1 -eq 1 ]; then
	nice ./work 1024 R 50000 & nice ./work 1024 R 10000
	sleep 1
	./monitor > profile1.data
elif [ $1 -eq 2 ]; then
	nice ./work 1024 R 50000 & nice ./work 1024 L 10000
	sleep 1
	./monitor > profile2.data
fi
