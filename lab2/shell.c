#include <stdlib.h>     /* Standard lib */
#include <string.h>     /* String functions */
#include <stdio.h>      /* Standard I/O */
#include <unistd.h>     /* System calls */
#include <sys/wait.h>   /* Wait for process to end */
#include <sys/stat.h>   /* Defines file permission modes */
#include <fcntl.h>      /* File descriptor */

#define MAX_LINE 80 /* 80 chars per line, per command */
#define MAX_ARGS (MAX_LINE / 2 + 1) /* command line (of 80) has max of 40 arguments */
#define DELIMITER " \n"

void init_args(char *args[]);
void refresh_args(char *args[]);
void init_command(char *command);
int get_input(char *command);
int parse_input(char *args[], char *original_command);
void run_command(char *args[], int args_num);
void check_pipe(char *args[], int *args_num, char **args2[], int *args_num2);
int io_redirection(char *args[], int *args_num);
void close_redirection(int fd);
void execute_command(char *args[]);

/**
 * main()
 * @void: void
 * 
 * A while loop that prints a shell prompt, gathers input, parses, and runs the command.
 * If the input is "exit" the loop breaks
 * 
 * Return: EXIT_SUCCESS
 */
int main(void) {
	char *args[MAX_ARGS];
	char command[MAX_LINE];
	int should_run = 1;

	init_args(args);
	init_command(command);
	while (should_run) {
		printf("osh>");
		fflush(stdout);
		fflush(stdin);

		refresh_args(args);

		if (!get_input(command)) continue;

		int args_num = parse_input(args, command);

        if (args_num == 0) continue;
        if (strcmp(args[0], "exit") == 0) break;

        run_command(args, args_num);
	}

    refresh_args(args);

	return EXIT_SUCCESS;
}

/**
 * init_args()
 * @args: command arguments array
 * 
 * Sets the arguments array pointers to NULL
 * 
 * Return: void
 */
void init_args(char *args[]) {
	int i;
    for(i = 0; i < MAX_ARGS; i++)
        args[i] = NULL;
}

/**
 * refresh_args()
 * @args: command arguments array
 * 
 * Clears out the arguments array and sets pointers to NULL
 * 
 * Return: void
 */
void refresh_args(char *args[]) {
    while(*args) {
        free(*args);
        *args++ = NULL;
    }
}

/**
 * init_command()
 * @command: command string
 * 
 * Sets command to an empty string.
 * 
 * Return: void
 */
void init_command(char *command) {
    strcpy(command, "");
}

/**
 * get_input()
 * @command: command string
 * 
 * Stores input from stdin into the input buffer.
 * 
 * If the input was "!!" check if @command contains a value
 *  If there was no previous command print "No commands in history."
 *  Otherwise, print the previous command and return success
 * Otherwise, the input is copied to the command 
 * 
 * Return: 1 on success 0 on failure
 */
int get_input(char *command) {
    char input_buffer[MAX_LINE];
    fgets(input_buffer, MAX_LINE, stdin);
	
    if(strncmp(input_buffer, "!!", 2) == 0) {
        if(strlen(command) == 0) {
            fprintf(stderr, "No commands in history.\n");
            return 0;
        }
        printf("%s", command);
        return 1;
    }
    strcpy(command, input_buffer);
    return 1;
}

/**
 * parse_input()
 * @args: command arguments array
 * @original_command: command string
 * 
 * Parses @original_command and creates a command argument array @args
 * 
 * Return: count of @args
 */
int parse_input(char *args[], char *original_command) {
    char command[MAX_LINE];
    strcpy(command, original_command);
    char *token = strtok(command, DELIMITER);

    int i;
    for (i = 0; token != NULL; i++) {
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        token = strtok(NULL, DELIMITER);
    }
    return i;
}

/**
 * run_command()
 * @args: first command arguments array
 * @args_num: count of first @args
 * 
 * Splits the args if there is a pipe
 * Creates a new process to run the command
 * If there is only one command setup redirection if necessary, exexute, and close.
 * If there is a pipe create another process
 * while calling pipe to create file descriptors to communicate with.
 * Execute the first command and output data that the second command takes in.
 * 
 * Return: void
 */
void run_command(char *args[], int args_num) {
    char **args2;
    int args_num2;

    args_num2 = 0;
    check_pipe(args, &args_num, &args2, &args_num2);

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed\n");
    } else if (pid == 0) {
        int iofd;

        if (args_num2 == 0) {
            iofd = io_redirection(args, &args_num);
            execute_command(args);
            close_redirection(iofd);
        } else {
            int fd[2];
            pipe(fd);

            pid_t pid2 = fork();
            if (pid < 0) {
                fprintf(stderr, "Fork Failed\n");
            } else if (pid2 == 0) {
                /* First Command */
                iofd = io_redirection(args, &args_num);
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                execute_command(args);
                close_redirection(iofd);
                close(fd[1]);
            } else {
                /* Second Command */
                iofd = io_redirection(args2, &args_num2);
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                wait(NULL);
                execute_command(args2);
                close_redirection(iofd);
                close(fd[0]);
            }
        }
    } else {
        wait(NULL);
    }
}

/**
 * check_pipe()
 * @args: first command arguments array
 * @args_num: count of first @args
 * @args2: second command arguments array
 * @args2_num: count of second @args
 * 
 * Loops through the @args and checks if it includes a  "|" pipe
 * If there is a pipe, it splits the arguments into the first and second command
 * 
 * Return: void
 */
void check_pipe(char *args[], int *args_num, char **args2[], int *args_num2) {
    int i;
    for (i = 0; i < *args_num; i++) {
        if (strcmp(args[i], "|") == 0) {
            free(args[i]);
            args[i] = NULL;

            *args2 = args + i + 1;
            *args_num2 = *args_num - i - 1;
            *args_num = i;
            break;
        }
    }
}

/**
 * io_redirection()
 * @args: command arguments array
 * @args_num: count of @args
 * 
 * Loops through the @args and checks if "<" input or ">" output is used.
 * Exctracts the pathname and removes it from args along with the sign
 * Creates the redirection from the standard input stream to a file by
 * duplicating the file descriptor.
 * 
 * Return: file descriptor used in redirection. -1 if none.
 */
int io_redirection(char *args[], int *args_num) {
    int fd = -1;

    int i;
    for (i = 0; i < *args_num; i++){
        if (strcmp("<", args[i]) == 0 && i < *args_num - 1) {
            char *pathname = args[i+1];

            mode_t read_mask = S_IRUSR | S_IRGRP | S_IROTH;

            fd = open(pathname, O_RDONLY, read_mask);
            dup2(fd, STDIN_FILENO);
            
            free(args[i]);
            free(args[i+1]);
            args[i] = NULL;
            args[i+1] = NULL;
            *args_num -= 2;

            break;
        }
        if (strcmp(">", args[i]) == 0 && i < *args_num - 1) {
            char *pathname = args[i+1];

            mode_t read_mask = S_IRUSR | S_IRGRP | S_IROTH;
            mode_t write_mask = S_IWUSR | S_IWGRP | S_IWOTH;

            fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, read_mask | write_mask);
            dup2(fd, STDOUT_FILENO);

            free(args[i]);
            free(args[i+1]);
            args[i] = NULL;
            args[i+1] = NULL;
            *args_num -= 2;

            break;
        }
    }

    return fd;
}

/**
 * close_redirection()
 * @fd: file descriptor
 * 
 * If redirection was in use, close @fd
 * 
 * Return: void
 */
void close_redirection(int fd) {
    if (fd != -1) close(fd);
}


/**
 * execute_command()
 * @args: command arguments array
 * 
 * uses execvp to execute the command defined in the @args
 * 
 * if there is an error execvp will return -1 and set errno
 * print "Command not found" after an error
 * the exit code should not be errno as the value could be greater than 255
 * 
 * Return: void
 */
void execute_command(char *args[]) {
    int err = execvp(args[0], args);
    if (err == -1) {
        fprintf(stderr, "Command not found\n");
        exit(EXIT_FAILURE);
    }
}
