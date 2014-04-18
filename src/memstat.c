#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>

#define SAMPLE_DELAY_DEFAULT (1000 * 500)

typedef struct meminfo {
	int used;
	char state;
} meminfo;

/* Parse Bytes
 * Converts from a byte string to an integer number of bytes
 * Ex: 2134 Kb -> 2185216
 */
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

/* Read file into buffer
 * Reads from the given fd into buf until the delim is found
 * or the file ends
 * TODO: dont buffer overflow?
 */
int read_into_buf(FILE *fd, char *buf, char delim) {
	char tmp;
	int i = 0;

	for(tmp = fgetc(fd); !feof(fd) && tmp != delim; tmp = fgetc(fd))
		buf[i++] = tmp;

	buf[i] = '\0';
	return !feof(fd);
}

/* Query the pseudo filesystem /proc for info about the given pid
 */
meminfo *meminfo_for_proc(int pid) {
	meminfo *mi;
	FILE *fi = NULL;
	char buf[512];
	char *statfile;

	mi = calloc(1, sizeof(meminfo));
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
		if (strncmp(buf,"VmRSS:",6) == 0) {
			mi->used = parse_bytes(buf + 6);
		}
		if (strncmp(buf,"State:", 6) == 0) {
			sscanf(buf + 6, " %c", &mi->state);
		}
	}
	fclose(fi);
	return mi;
}

void monitor_process(int pid) {
	int sample_delay = 1000 * 500;
	FILE *out_fi = stdout;
	meminfo *mi = NULL;
	char *temp = NULL;

	//Check for user defined sample delay
	temp = getenv("MEMSTAT_SAMPLE_DELAY");
	if (temp != NULL) {
		sample_delay = atoi(temp);
		if (sample_delay <= 0) {
			sample_delay = SAMPLE_DELAY_DEFAULT;
		}
	}

	//Check for user defined log file
	temp = getenv("MEMSTAT_LOG_FILE");
	if (temp != NULL) {
		out_fi = fopen(temp, "w");
		if (out_fi == NULL) {
			out_fi = stdout;
		}
	}

	fprintf(out_fi, "Starting monitoring on process PID=%d\n", pid);
	while ((mi = meminfo_for_proc(pid)) != NULL && kill(pid,0) == 0) {
		fprintf(out_fi, "time: %ld, ", time(NULL));
		fprintf(out_fi, "mem:%d\n", mi->used);
		fflush(out_fi);
		usleep(sample_delay);
		if (mi->state == 'Z') {
			fprintf(out_fi, "Process is zombie, exiting!\n");
			break;
		}
		free(mi);
	}
	fclose(out_fi);
}

int main (int argc, char ** argv, char ** env)
{
	int pid = fork();
	if (pid == 0) {
		execve(*(argv + 1), argv + 1, env);
	} else {
		monitor_process(pid);
	}

	return 0;
}
