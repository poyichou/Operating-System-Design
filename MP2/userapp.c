#include<stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "userapp.h"

int ifexist(int mypid){
	
	FILE *fp = fopen("/proc/mp2/status", "r");
	if(fp == NULL){
		perror("fopen r");
		exit(1);
	}
	int pid, state;
	unsigned long period, computation;
	while(fscanf(fp, "%d %lu %lu %d", &pid, &computation, &period, &state) != EOF){
		//printf("pid=%d\n", pid);
		if(mypid == pid){
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);
	return -1;
}
void call_yield(int pid){
	FILE *fp = fopen("/proc/mp2/status", "w");
	if(fp == NULL){
		perror("fopen w");
		exit(1);
	}
	fprintf(fp, "Y %d", (int)pid);
	fclose(fp);
}
void deregister(int pid){
	FILE *fp = fopen("/proc/mp2/status", "w");
	if(fp == NULL){
		perror("fopen w");
		exit(1);
	}
	fprintf(fp, "D %d", (int)pid);
	fclose(fp);
}
void job(){
	int i;
	for(i = 0 ; i < 100000 ; i++);
}
int main(int argc, char* argv[])
{
	unsigned long period = 600;
	unsigned long computation = 100;
	unsigned long jobs = 5;
	if(argc == 4){
		period = atoi(argv[1]);
		computation = atoi(argv[2]);
		jobs = atoi(argv[3]);
	}else if(argc != 1){
		printf("usage: [programe] [period] [computation] [jobs] or [program]");
		return 0;
	}
	FILE *fp = fopen("/proc/mp2/status", "w");
	if(fp == NULL){
		perror("fopen w");
		return 1;
	}

	pid_t pid = getpid();
	//register
	fprintf(fp, "R %d %lu %lu", (int)pid, period, computation);
	fclose(fp);
	//fprintf(stdout, "%d", (int)pid);
	if(ifexist((int)pid) == -1){
		printf("registration failed");
		return 0;
	}
	struct timeval  tv1, tv2;
	gettimeofday(&tv1, NULL);
	double last_time = tv1.tv_sec, spent;
	call_yield((int)pid);
	int time = 0;
	while(time < jobs){
		//job
		gettimeofday(&tv2, NULL);
		printf("pid: %d, start time:\t%d ms\n", (int)pid, (int)(tv2.tv_sec * 1000));
		last_time = tv2.tv_sec;
		time++;
		job();
		gettimeofday(&tv2, NULL);
		spent = tv2.tv_sec - last_time;
		printf("pid: %d, finish time:\t%d ms\n", (int)pid, (int)(tv2.tv_sec * 1000));
		call_yield((int)pid);
	}
	deregister((int)pid);
	return 0;
}
