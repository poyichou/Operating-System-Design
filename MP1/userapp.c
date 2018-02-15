#include<stdio.h>
#include <sys/types.h>
#include <unistd.h>
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
	return 0;
}
