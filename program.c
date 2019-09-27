#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LENGTH 256

#define MAX_LINE 80 /* The maximum length of a command */
#define BUFFER_SIZE 50
#define buffer "\n\Shell Command History:\n"

// Khai bao
char history[100][BUFFER_SIZE]; //history array to store history commands
int count = 0;

// Xu li chuoi da nhap vao
void parse(char* commandline, char **args, char* commandArgToken, bool *hasAmpersand)
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
		if (strcmp(args[argCount - 1], "&") == 0)
		{
			args[argCount - 1] = NULL;
			*hasAmpersand = true;
		}
}

void execute(char** args, bool hasAmpersand)
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
		// Truyen cac tham so da luu trong args vao execvp

		if (execvp(args[0], args) < 0)
		{
			printf("[ERROR]: Exec failed\n");
			exit(1);
		}
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

// Hàm này sẽ chỉ in 100 lệnh mới nhất trong lịch sử
void displayHistory()
{

	printf("Shell command history:\n");

	int i;
	int j = 0;
	int histCount = count;

	//loop for iterating through commands
	for (i = 0; i < 100; i++)
	{
		//command index
		printf("%d.  ", histCount);
		while (history[i][j] != '\n' && history[i][j] != '\0')
		{
			//printing command
			printf("%c", history[i][j]);
			j++;
		}
		printf("\n");
		j = 0;
		histCount--;
		if (histCount == 0)
			break;
	}
	printf("\n");
}

//Fuction to get the command from shell, tokenize it and set the args parameter
int formatCommand(char inputBuffer[], char *args[], bool *hasAmpersand)
{
	int length; // # of chars in command line
	int i;     // loop index for inputBuffer
	int start;  // index of beginning of next command
	int ct = 0; // index of where to place the next parameter into args[]
	int hist;
	//read user input on command line and checking whether the command is !! or !n

	length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

	start = -1;

	if (length == 0)
		exit(0);   //end of command

	if (length < 0)
	{
		printf("Command not read\n");
		exit(-1);
	}

	//kiem tra tung ki tu
	for (i = 0; i < length; i++)
	{
		switch (inputBuffer[i])
		{
		case ' ':
		case '\t':               // to seperate arguments
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0'; // add a null char at the end
			start = -1;
			break;

		case '\n':                 //final char 
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; // no more args
			break;

		default:
			if (start == -1)
				start = i;
			if (inputBuffer[i] == '&')
			{
				*hasAmpersand = true; //this flag is the differentiate whether the child process is invoked in background
				inputBuffer[i] = '\0';
			}
		}
	}

	args[ct] = NULL; //if the input line was > 80

	if (strcmp(args[0], "history") == 0)
	{
		if (count > 0)
		{
			displayHistory();
		}
		else
		{
			printf("\nNo Commands in the history\n");
		}
		return -1;
	}

	else if (args[0][0] - '!' == 0)
	{
		int x = args[0][1] - '0';
		int z = args[0][2] - '0';

		if (x > count) //second letter check
		{
			printf("\nNo Such Command in the history\n");
			strcpy(inputBuffer, "Wrong command");
		}
		else if (z != -48) //third letter check 
		{
			printf("\nNo Such Command in the history. Enter <=!99 (buffer size is 100 along with current command)\n");
			strcpy(inputBuffer, "Wrong command");
		}
		else
		{
			if (x == -15)//Checking for '!!',ascii value of '!' is 33.
			{
				strcpy(inputBuffer, history[0]);  // this will be your 100 th(last) command
			}
			else if (x == 0) //Checking for '!0'
			{
				printf("Enter proper command");
				strcpy(inputBuffer, "Wrong command");
			}

			else if (x >= 1) //Checking for '!n', n >=1
			{
				strcpy(inputBuffer, history[count - x]);

			}
		}
	}
	for (i = 99; i > 0; i--) //Moving the history elements one step higher
		strcpy(history[i], history[i - 1]);

	strcpy(history[0], inputBuffer); //Updating the history array with input buffer
	count++;
	if (count > 100)
	{
		count = 100;
	}
}

void Run_history()
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the input command */
	bool hasAmpersand; // equals true if a command is followed by "&"
	char *args[MAX_LINE / 2 + 1];/* max arguments */
	bool should_run = true;

	pid_t pid, tpid;
	int i;

	while (should_run) //infinite loop for shell prompt
	{
		hasAmpersand = false; //*hasAmpersand = false by default
		printf("osh->");
		fflush(stdout);
		if (formatCommand(inputBuffer, args, &hasAmpersand) != -1) // get next command  
		{
			pid = fork();

			if (pid < 0)//if pid is less than 0, forking fails
			{
				printf("Fork failed.\n");
				exit(1);
			}

			else if (pid == 0)//if pid ==0
			{
				//command not executed
				if (execvp(args[0], args) == -1)
				{
					printf("Error executing command\n");
				}
			}

			// if hasAmpersand == false, the parent will wait,
			// otherwise returns to the formatCommand() function.
			else
			{
				i++;
				if (hasAmpersand == false)
				{
					i++;
					wait(NULL);
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	//bool should_run = true;
	//while (should_run) {
	//	int temp;
	//	printf("osh->");

	//	// Doc dong lenh duoc nhap vao
	//	char commandline[MAX_LENGTH];
	//	fgets(commandline, MAX_LENGTH, stdin);
	//	commandline[strlen(commandline) - 1] = '\0';

	//	// Neu command la exit thi thoat
	//	if (strcmp(commandline, "exit") == 0) exit(0);

	//	char* commandArgToken = NULL;
	//	// Mang luu cac tham so de truyen vao execvp
	//	char* args[64];
	//	bool hasAmpersand = false;

	//	// Xu li chuoi lenh da nhap
	//	parse(commandline, args, commandArgToken, &hasAmpersand);

	//	// Thuc thi lenh da nhap
	//	execute(args, hasAmpersand);
	//}
	Run_history();
	return 0;
}

