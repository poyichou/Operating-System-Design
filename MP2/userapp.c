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
int main(int argc, char* argv[])
{
	unsigned long period = 60;
	unsigned long computation = 10;
	if(argc == 3){
		period = atoi(argv[1]);
		computation = atoi(argv[2]);
	}else if(argc != 1){
		printf("usage: [programe] [period] [computation] or [program]");
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
	while(time < 20){
		//job
		gettimeofday(&tv2, NULL);
		spent = tv2.tv_sec - last_time;
		last_time = tv2.tv_sec;
		time++;
		printf("pid: %d, wake up time: %lf, timespent: %lf\n", (int)pid, (double)tv2.tv_sec, spent);
		call_yield((int)pid);
	}
	deregister((int)pid);
	return 0;
}
