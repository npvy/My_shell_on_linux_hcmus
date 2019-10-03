#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LENGTH 256
#define MAX_ARGS 64

#define MAX_LINE 80 /* The maximum length of a command */
#define BUFFER_SIZE 50
#define HISTORY_LIMIT 100

#define REDIRECTION_MODE_NONE 0
#define REDIRECTION_MODE_IN 1
#define REDIRECTION_MODE_OUT 2

// Xu li chuoi da nhap vao
void parse(char* commandline, char **args, char* commandArgToken, bool *hasAmpersand, int *redirectionMode)
{
	const char* delim = " ";
	commandArgToken = strtok(commandline, delim);

	// Lan luot gan cac tham so da nhap trong commandline vao mang args 
	int i = 0;
	int argCount = 0;
	while (commandArgToken != NULL) 
	{
		args[i] = commandArgToken;
		commandArgToken = strtok(NULL, delim);
		i++;
		argCount++;
	}
	// Phan tu cuoi cung cua mang args phai la NULL
	args[i] = NULL;

	if (argCount > 1)
	{
		if (strcmp(args[argCount - 1], "&") == 0)
		{
			args[argCount - 1] = NULL;
			*hasAmpersand = true;
			argCount--;
		}

		if (argCount > 2)
		{
			if (strcmp(args[argCount - 2], "<") == 0) *redirectionMode = REDIRECTION_MODE_IN;
			if (strcmp(args[argCount - 2], ">") == 0) *redirectionMode = REDIRECTION_MODE_OUT;
		}
	}
}

void execute(char** args, bool hasAmpersand, int redirectionMode)
{
	pid_t childPID = fork();
	pid_t pid;

	int status;
	if (childPID < 0)
	{
		// Truong hop khong fork duoc child process
		printf("[ERROR]: Forking child process failed!\n");
		exit(1);
	}
	else if (childPID == 0)
	{
		// Truong hop process dang chay la child process

		int fd;
		if (redirectionMode = REDIRECTION_MODE_OUT)
		{
			fd = open("output.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
			if(fd == -1) perror("fd: ");
			dup2(fd, 1);
		}

		// Truyen cac tham so da luu trong args vao execvp

		if (execvp(args[0], args) < 0)
		{
			printf("[ERROR]: Exec failed\n");
			exit(1);
		}

		close(fd);
	}
	else 
	{
		// Truong hop process dang chay la parent process

		// Neu trong cac tham so da nhap khong co dau &
		if (!hasAmpersand)
		while (wait(&status) != childPID); // Parent process phai cho` child process hoan tat
	}
}

//Bo sung
void add_history(char* history[], char* commandline, int* count) 
{
	int i;

	// Store the issued command into the history array, removing the first value and shifting the rest of the values over if necessary.
	for (i = 0; i < HISTORY_LIMIT; i++)
	{

		// If the history has already reached its limit, then copy the value from the next value.
		if (*count >= HISTORY_LIMIT) 
		{
			// If the index is not the last index, then copy the value from the next value.
			if (i < HISTORY_LIMIT - 1)
			{
				strcpy(history[i], history[i + 1]);
			}

			// Copy the new command to the last index.
			else 
			{
				strcpy(history[i], commandline);
			}
		}

		// If the history has not reached the limit, then copy the new command to the first available index.
		else if (!strcmp(history[i], "\0"))
		{
			strcpy(history[i], commandline);
			break;
		}
	}
	*count += 1; // Increment the history count.
}

int main(int argc, char* argv[])
{
	bool should_run = true;
	char commandline[MAX_LENGTH];

	// Initialize command history array.
	int history_count = 0;
	char* history[HISTORY_LIMIT];

	int i;
	for (i = 0; i < HISTORY_LIMIT; i++) 
	{
		history[i] = malloc(MAX_LINE);
		strcpy(history[i], "\0");
	}

	while (should_run) 
	{
		int temp;
		printf("osh->");

		// Doc dong lenh duoc nhap vao
		fgets(commandline, MAX_LENGTH, stdin);
		fflush(stdin);

		commandline[strlen(commandline) - 1] = '\0';

		// Neu command la exit thi thoat
		if (strcmp(commandline, "exit") == 0)
		{
			should_run = false;
			exit(0);
		}
		if (!strcmp(commandline, "history"))
		{
			for (i = HISTORY_LIMIT - 1; i >= 0; i--)
			{
				if (!strcmp(history[i], "\0"))
				{
					continue;
				}
				printf("%i %s\n", i + 1 + (history_count > HISTORY_LIMIT ? history_count - HISTORY_LIMIT : 0), history[i]);
			}
			continue;
		}

		// Handle "!!" commandline.
		else if (!strcmp(commandline, "!!")) 
		{
			if (history_count == 0) 
			{
				printf("No commands in history.\n");
				continue;
			}
			else 
			{
				strcpy(commandline, history_count > HISTORY_LIMIT ? history[HISTORY_LIMIT - 1] : history[history_count - 1]);
				printf("%s\n", commandline);
			}
		}
		// Store user commandline in history, even if its an invalid command. The "history" command is not stored.
		add_history(history, commandline, &history_count);

		char* commandArgToken = NULL;
		// Mang luu cac tham so de truyen vao execvp
		char* args[MAX_ARGS];
		bool hasAmpersand = false;

		// Flag to handle command differently if it contains a redirection operator "<" or ">"
		int redirectionMode = REDIRECTION_MODE_NONE; // default is NONE

		// Xu li chuoi lenh da nhap
		parse(commandline, args, commandArgToken, &hasAmpersand, &redirectionMode, &redirectedFilepath);

		// Thuc thi lenh da nhap
		execute(args, hasAmpersand, redirectionMode);
	}

	return 0;
}
