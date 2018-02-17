#include<stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "userapp.h"

int main(int argc, char* argv[])
{
	FILE *fp = fopen("/proc/mp1/status", "w");
	if(fp == NULL){
		perror("fopen");
		return 1;
	}
	pid_t pid = getpid();
	fprintf(fp, "%d", (int)pid);
	fclose(fp);
	//fprintf(stdout, "%d", (int)pid);
	struct timeval  tv1, tv2;
	gettimeofday(&tv1, NULL);
	while(1){
		gettimeofday(&tv2, NULL);
		//time out
		if((double)(tv2.tv_sec - tv1.tv_sec) >= 12.0){
			break;
		}
	}
	execlp("cat", "cat", "/proc/mp1/status", (char*)NULL);
	return 0;
}
