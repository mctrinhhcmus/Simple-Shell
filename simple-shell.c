/*
 * Project 1: Simple Shell
 * Team: TOMAHAWK
 ** Mai Cong Trinh - 1712840
 ** Tran Dinh Sang - 1712722
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>

#define NUM_ARGS 20
#define LEN_ARG 20
#define LEN_CMD 50
#define CMD_HIS "history"
#define CMD_EXIT "exit"
#define CMD_NO_WAIT "&"
#define CMD_CD "cd"
#define CMD_PRE "!!"
#define CMD_IN "<"
#define CMD_OUT ">"
#define CMD_PIPE "|"
#define PROMPT "osh>"
#define TOKEN " "
#define END_STR '\0'
#define IN_CHILD(x) x == 0
#define IN_PARENT(x) x > 0
#define MAX_HIS 20
#define OUT_MODE O_RDWR | O_CREAT | S_IRWXU
#define IN_MODE O_RDONLY

enum MSG
{
	PS_NO_CREATE,
	CMD_NO_EXEC,
	HIS_EMPTY,
	FILE_NO_CREATE,
	FILE_NO_OPEN,
	FD_NO_REDIRECT,
	DIR_ERROR
};

const char *msg[] = {
	"Process is not create.",
	"Command is not execute.",
	"No commands in history.",
	"Can't create output file.",
	"No input file.",
	"Can't redirect.",
	"Directory can't open."
};

/*
 * Find arg in array
 * Return position arg, otherwise return -1
 */
int findArg(char* args[], int numArgs, const char argX[])
{
	int j;
	for (j = 0; j < numArgs; j++)
	{
		if (args[j] == NULL)
			continue;

		if (strcmp(args[j], argX) == 0)
			return j;
	}

	return -1;
}

/*
 * Parsing command
 * Return 0 if success
 */
int parsingCmd(char src[LEN_CMD], char* des[NUM_ARGS], int* numArgs)
{
	char *rest = (char*)malloc(strlen(src));
	strcpy(rest, src);
	int i = 0;

	// Using strtok_r
	while ((des[i] = strtok_r(rest, TOKEN, &rest)) != NULL)
	{
		i++;
	}

	// Last pointer
	des[i] = NULL;
	*numArgs = i;

	// Return 0 if success
	return 0;
}

/*
 * CMD history
 * Print out 20 most recent cmd lists
 * Return 0 if success
 */
int outHistory(char history[LEN_CMD][MAX_HIS], int numCmd)
{
	int i,j;
	if (numCmd == 0)
	{
		puts(msg[HIS_EMPTY]);
	}
	else if (numCmd <= 20)
	{
		for (i = 0; i< numCmd; i++)
		{
			printf("%d: %s\n", i, history[i]);
		}
	}
	else
	{
		int j = numCmd - MAX_HIS;
		for (i = 0; i < MAX_HIS; i++)
		{
			printf("%d: %s\n", j, history[j % MAX_HIS]);
			j++;
		}
	}
	return 0;
}

/*
 * Redirect output file
 * Return the file that was redirected to write
 */
int outputFile(char filename[])
{
	int des = creat(filename, OUT_MODE);

	if (des < 0)
	{
		puts(msg[FILE_NO_CREATE]);
		exit(2);
	}

	if (dup2(des, STDOUT_FILENO) < 0)
	{
		puts(msg[FD_NO_REDIRECT]);
		exit(2);
	}

	return des;
}
/*
 * Redirect input file
 * Return the file that was redirected to read
 */
int inputFile(char filename[])
{
	int src = open(filename, IN_MODE);

	if (src < 0)
	{
		puts(msg[FILE_NO_OPEN]);
		exit(2);
	}

	if (dup2(src, STDIN_FILENO) < 0)
	{
		puts(msg[FD_NO_REDIRECT]);
		exit(2);
	}

	return src;
}

/*
 * Pipe: to allow the output of one command to serve as input to another
 * Return 0 if success
 */
int piping(char* args[], int numArgs)
{
	char* args2[LEN_ARG];
	int p[2];
	int j;

	if (pipe(p) < 0)
		exit(2);

	int pidProcess2 = fork();

	if (IN_CHILD(pidProcess2))
	{
		if (dup2(p[0], STDIN_FILENO) < 0)
		{
			puts(msg[FD_NO_REDIRECT]);
			exit(2);
		}
		close(p[0]);
		close(p[1]);

		for (j = 0; j < numArgs; j++)
		{
			args2[j] = args[numArgs - 1 + j];
		}
		args2[j] = NULL;

		execvp(args[numArgs - 1], args2);
		puts(msg[CMD_NO_EXEC]);
		exit(2);
	}
	else if (IN_PARENT(pidProcess2))
	{
		pidProcess2 = fork();

		if (IN_CHILD(pidProcess2))
		{
			if (dup2(p[1], STDOUT_FILENO) < 0)
			{
				puts(msg[FD_NO_REDIRECT]);
				exit(2);
			}

			close(p[1]);
			close(p[0]);
			args[numArgs - 2] = NULL;
			execvp(args[0], args);
			puts(msg[CMD_NO_EXEC]);
			exit(2);
		}
		else if (IN_PARENT(pidProcess2))
		{
			int status;
			close(p[0]);
			close(p[1]);
			waitpid(pidProcess2, &status, 0);
		}
		else
		{
			puts(msg[PS_NO_CREATE]);
		}

		wait(NULL);
	}
	else
	{
		puts(msg[PS_NO_CREATE]);
	}

	return 0;
}

/*
 * Execute command: 
 ** redirecting Input and Output
 ** history
 ** pipe and exit
 * Return 0 if success
 */
int executeCmd(char* args[NUM_ARGS], int numArgs, char history[LEN_CMD][MAX_HIS], int numCmd)
{
	int checkArg;
	// Output file
	if (numArgs > 2 && (checkArg = findArg(args, numArgs, CMD_OUT)) > -1)
	{
		puts(args[checkArg + 1]);
		outputFile(args[checkArg + 1]);
		args[checkArg] = NULL;
	}
	// Input
	if (numArgs > 2 && (checkArg = findArg(args, numArgs, CMD_IN)) > -1)
	{
		puts(args[checkArg + 1]);
		inputFile(args[checkArg + 1]);
		args[checkArg] = NULL;
	}
	// Pipe and exit
	if (numArgs > 2 && (checkArg = findArg(args, numArgs, CMD_PIPE)) > -1)
	{
		piping(args, numArgs);
	}
	// History
	if (strcmp(args[0], CMD_HIS) == 0)
	{
		outHistory(history, numCmd);
		exit(1);
	}
	// Normal
	else
	{
		execvp(args[0], args);
		puts(msg[CMD_NO_EXEC]);
		exit(2);
	}

	return 0;
}

int main()
{
	int running = 1; // Running flag
	int numCmd = 0; // Total cmd
	char command[LEN_CMD];	// Store cmd string
	char history[LEN_CMD][MAX_HIS]; // Array history
	char* args[LEN_CMD]; // Store parsing cmd
	int numArgs = 0; // Total args when parsing cmd
	int flagWait = 1;
	int checkArg; // Check cmd wait
	char dirName[10];
	char* dir = dirName;
	pid_t pidProcess; // Process identifier
	char cwd[PATH_MAX]; // Current address

	while (running)
	{

		flagWait = 1;
		printf(PROMPT);

		if (getcwd(cwd, sizeof(cwd)) == NULL)
		{
			puts(msg[DIR_ERROR]);
			continue;
		}

		printf("%s>", basename(cwd));
		fflush(stdout);

		// Get command
		fgets(command, LEN_CMD, stdin);
		command[strlen(command) - 1] = END_STR;

		// Check cmd

		// Parsing
		parsingCmd(command, args, &numArgs);

		// Check empty
		if (numArgs == 0)
		{
			continue;
		}
		// Check pre command
		if (strcmp(args[0], CMD_PRE) == 0)
		{
			if (numCmd == 0)
			{
				puts(msg[HIS_EMPTY]);
				continue;
			}
			strcpy(command, history[(numCmd - 1) % MAX_HIS]);
			parsingCmd(command, args, &numArgs);
		}
		// Strore command in History
		strcpy(history[numCmd % MAX_HIS], command);
		numCmd++;

		// Check terminated shell
		if (strcmp(args[0], CMD_EXIT) == 0)
		{
			running = 0;
			continue;
		}
		if (strcmp(args[0], CMD_CD) == 0)
		{
			if (chdir(args[1]) < 0)
			{
				puts(msg[DIR_ERROR]);
			}
			continue;
		}

		// Check wait
		if ((checkArg = findArg(args, numArgs, CMD_NO_WAIT)) > -1)
		{
			flagWait = 0;
			args[checkArg] = NULL;
		}
		// Duplicate process
		pidProcess = fork();

		// Check
		if (IN_CHILD(pidProcess))
		{
			executeCmd(args, numArgs, history, numCmd);
		}
		else if (IN_PARENT(pidProcess))
		{
			// Wait child process terminated
			//
			if (flagWait == 0)
			{
				continue;
			}
			waitpid(pidProcess, NULL, 0);
		}
		else
		{
			// Put msg
			puts(msg[PS_NO_CREATE]);
		}
	}

	return 0;
}
