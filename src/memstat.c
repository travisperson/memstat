#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct meminfo {
	int used;
} meminfo;

int parse_bytes(char *s) {
	int num;

	//Skip leading whitespace
	for (;*s == ' '||*s == '\t';s++);
	
	num = atoi(s);

	//now skip over the number
	for (;*s != ' ';s++);
	s++;
	switch (*s) {
		case 'k':
		case 'K':
			num *= 1024;
			break;
		case 'm':
		case 'M':
			num *= (1024 * 1024);
			break;
		case 'g':
		case 'G':
			num *= (1024 * 1024 * 1024);
			break;
	}
	return num;
}

int read_into_buf(FILE *fd, char *buf, char delim) {
	char tmp;
	int i = 0;

	for(tmp = fgetc(fd); !feof(fd) && tmp != delim; tmp = fgetc(fd))
		buf[i++] = tmp;

	buf[i] = '\0';
	return !feof(fd);
}

meminfo *meminfo_for_proc(int pid) {
	meminfo *mi;
	FILE *fi = NULL;
	char buf[512];
	char *statfile;

	mi = malloc(sizeof(meminfo));
	if (mi == NULL) {
		//panic
		printf("Failed to allocate memory for structure.\n");
		return NULL;
	}

	asprintf(&statfile, "/proc/%d/status", pid);
	fi = fopen(statfile,"r");
	if (fi == NULL) {
		//also panic
		printf("Failed to open file.\n");
		return NULL;
	}

	while (read_into_buf(fi, buf, '\n')) {
		if (strncmp(buf,"VmSize:",7) == 0) {
			mi->used = parse_bytes(buf + 7);
		}
	}
	return mi;
}


int main (int argc, char ** argv, char ** env)
{
	int pid = fork();
	int sample_rate = 1000 * 500;
	meminfo *mi = NULL;

	if ( pid == 0 )
	{
		execve(*(argv + 1), argv + 1, env);
	}
	else
	{
		// Parent
		printf("Starting monitoring on process PID=%d\n", pid);
		while ((mi = meminfo_for_proc(pid)) != NULL && kill(pid,0) == 0) {
			printf("%d\n", mi->used);
			usleep(sample_rate);
		}
	}

	return 0;
}
