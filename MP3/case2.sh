#!/bin/bash
#usage: [program] [num]
num=$1  
for ((i=0; i < num - 1; i++))  
do  
	nice ./work 200 R 10000 &  
done  

nice ./work 200 R 10000
sleep 1

if [ $1 -eq 1 ]; then
	./monitor > profile3.data
elif [ $1 -eq 5 ]; then
	./monitor > profile4.data
elif [ $1 -eq 11 ]; then
	./monitor > profile5.data
fi
