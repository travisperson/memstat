#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <unistd.h>


int main (int argc, char ** argv, char ** env)
{

	int pid = fork();

	if ( pid == 0 )
	{
		execve(*(argv + 1), argv + 1, env);
	}
	else
	{
		// Parent
		printf("Starting monitoring on process PID=%d\n", pid);
		wait(NULL);
	}

	return 0;
}
