#!/bin/bash
#usage: [program] [num]
num=$1  
for ((i=0; i < num; i++))  
do  
	nice ./work 200 R 10000 &  
done  
