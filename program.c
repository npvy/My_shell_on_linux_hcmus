#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define BUFSIZE 64     // Size of a single token
#define TOKEN_DELIM " \t\r\n\a"  // Token Delimiters
#define HISTORY_LIMIT 100
#define MAX_LINE 80 /* The maximum length of a command */

//History
void Add_history(char* history[], char* commandline, int* count)
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

/* IORedirect function redirects Standard Input or Standard Output depending on value of parameter ioMode */
void IORedirect(char **args, int i, int ioMode)
{
	pid_t pid, wpid;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fd, status = 0;

	pid = fork();
	if (pid == 0)
	{
		// Child process
		if (ioMode == 0)   // Input mode
			fd = open(args[i + 1], O_RDONLY, mode);
		else              // Output mode
			fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, mode);
		if (fd < 0)
			fprintf(stderr, "File error");
		else {

			dup2(fd, ioMode);   // Redirect input or output according to ioMode
			close(fd);          // Close the corresponding pointer so child process can use it
			args[i] = NULL;
			args[i + 1] = NULL;

			if (execvp(args[0], args) == -1)
			{
				perror("SHELL");
			}
			exit(EXIT_FAILURE);
		}
	}
	else if (pid < 0)
	{ // Error forking process
		perror("SHELL");
	}
	else
	{
		// Parent process. Wait till it finishes execution
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}

// Function for Pipe
/* The CopyArgs function is used to selectively copy an array of strings into another array.
* Parameter argc specifies copy limit */
static char** CopyArgs(int argc, char **args)
{
	char **copy_of_args = malloc(sizeof(char *)* (argc + 1));
	int i;
	for (i = 0; i < argc; i++)
	{
		copy_of_args[i] = strdup(args[i]);
	}
	copy_of_args[i] = 0;
	return copy_of_args;
}

void PipeRedirect(char **args, int i)
{
	int fpipe[2];
	char **copy_args;

	copy_args = CopyArgs(i, args);
	if (pipe(fpipe) == -1)
	{  // Create a pipe with one input pointer and one output pointer
		perror("pipe redirection failed");
		return;
	}

	if (fork() == 0)   // child 1        
	{
		dup2(fpipe[1], STDOUT_FILENO);   // Redirect STDOUT to output part of pipe     
		close(fpipe[0]);      //close pipe read
		close(fpipe[1]);      //close write pipe

		execvp(copy_args[0], copy_args);    // pass the truncated command line as argument
		perror("First program execution failed");
		exit(1);
	}

	if (fork() == 0)   // child 2
	{
		dup2(fpipe[0], STDIN_FILENO);   // Redirect STDIN to Input part of pipe         
		close(fpipe[1]);       //closing pipe write
		close(fpipe[0]);       //close read pipe 

		execvp(args[i + 1], args + i + 1);    // pass the second part of command line as argument
		perror("Second program execution failed");
		exit(1);
	}

	close(fpipe[0]);
	close(fpipe[1]);
	wait(0);   // Wait for child 1 to finish
	wait(0);   // Wait for child 2
}

/* The Cmd function takes a command line as input and splits it into tokens
* delimited by the standard delimiters */
char **Cmd(char *cmd)
{
	int bufsize = BUFSIZE, i = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "Memory allocation failure\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(cmd, TOKEN_DELIM);  // Get the first token
	while (token != NULL)
	{
		tokens[i] = token;
		i++;

		if (i >= bufsize) 
		{     // If number of tokens are more then reallocate
			bufsize += BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens)
			{
				fprintf(stderr, "memory allocation failure\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, TOKEN_DELIM);
	}
	tokens[i] = NULL;  // indicate token end
	return tokens;
}

void StartProcess(char **args, int i)
{
	pid_t pid, wpid;
	int status;

	if (i > 0)
		args[i] = NULL; // Set the location of '&' to NULL in order to pass it to execvp()
	pid = fork();
	if (pid == 0) // fork success. child initiated          
	{
		execvp(args[0], args);
		perror("Program execution failed");
		if (i == 0)
			exit(1);
	}
	if (i == 0)
	{
		// If not a background process, wait till it finishes execution.
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	else
	{
		// For a background process, do not wait indefinitely, but return to prompt
		fprintf(stderr, "Starting background process...");
		wpid = waitpid(-1, &status, WNOHANG);
	}
}

void main()
{
	char *commandline = NULL, *pwd;
	char **args, *options[4] = { "<", ">", "|", "&" };
	int i = 0, j = 0, l, found;

	pwd = NULL;

	// Initialize command history array.
	int history_count = 0;
	char* history[HISTORY_LIMIT];

	for (j = 0; j < HISTORY_LIMIT; j++)
	{
		history[j] = malloc(MAX_LINE);
		strcpy(history[j], "\0");
	}

	// Get the home directory of the user
	pwd = getenv("HOME");

	printf("\n***** Unix/Linux Command *****\n\n");
	int should run = 1; /* flag to determine when to exit program */
	while (should run) 
	{
		ssize_t bsize = 0;
		found = 0;

		printf("osh->");
		getline(&commandline, &bsize, stdin);
		args = Cmd(commandline);             // Split command line into tokens

		if (args[0] == NULL) 
		{
			free(commandline);
			free(args);
			continue;
		}
		if (strcmp(args[0], "exit") == 0)        // Handle the exit command. Break from command loop
			break;
		else if (strcmp(args[0], "cd") == 0)
		{   // Handle cd command.
			if (args[1] == NULL) 

			{              // If no argument specified in cd change to home
				if (pwd[0] != 0) 
				{              // directory. 
					chdir(pwd);
				}
			}
			else 
			{
				chdir(args[1]);
			}
		}
		else if (!strcmp(args[0], "history"))
		{
			for (j = HISTORY_LIMIT - 1; j >= 0; j--)
			{
				if (!strcmp(history[j], "\0"))
				{
					continue;
				}
				printf("%i %s\n", j + 1 + (history_count > HISTORY_LIMIT ? history_count - HISTORY_LIMIT : 0), history[j]);
			}
			continue;
		}

		// Handle "!!" commandline.
		else if (!strcmp(args[0], "!!"))
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
				StartProcess(args, 0);                      // Start a foreground process or command
			}
		}
		else 
		{
			i = 1;
			while (args[i] != NULL)
			{           // Check for any of the redirect or process operators <,<,|,&
				for (l = 0; l < 4; l++) 
				{
					if (strcmp(args[i], options[l]) == 0)
						break;
				}
				if (l < 4)
				{
					found = 1;
					if (l< 3 && args[i + 1] == NULL) 
					{   // For IORedirect and Pipe, argument is necessary
						fprintf(stderr, "SHELL: parameter missing\n");
						break;
					}
					if (l < 2)                          // Redirect IO,l=0 for Input, l=1 for Output
						IORedirect(args, i, l);
					else if (l == 2)
						PipeRedirect(args, i);              // Pipe Redirection
					else if (l == 3)
						StartProcess(args, i);              // Start a background process
					break;
				}

				i++;
			}
			if (found == 0)
				StartProcess(args, 0);                      // Start a foreground process or command
		}

		
		// Store user commandline in history, even if its an invalid command. The "history" command is not stored.
		Add_history(history, commandline, &history_count);

		free(commandline);
		free(args);
	} 
}
