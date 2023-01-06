#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "global.h"
// For isalnum(int character)
#include <ctype.h>

#define MAX_SIZE 1024
typedef struct var {
    char* key;
    char* value;
} var;

// My local environment
var* env[MAX_SIZE] = {0};
// Where is the first available spot to add a variable
int indice = 0;

// User name and working environment
char usr[MAX_SIZE] = {0};
char hostname[MAX_SIZE] = {0};

// In order to get the username
#include <pwd.h>
char* getUserName()
{
	uid_t uid = geteuid();
	struct passwd *pw = getpwuid(uid);
	return pw? pw -> pw_name : "";
}

// Quick declaration
int execute (struct cmd *cmd);

// name of the program, to be printed in several places (nope I made up my mind and it disapeared)
#define NAME "myshell"

// Some helpful functions
void errmsg (char *msg)
{
	fprintf(stderr,"error: %s\n",msg);
}

// Search in the environment the variable called var_name
var* search(char* var_name)
{
	for (int k = 0; k<indice; k++)
	{
		if (!strcmp(env[k] -> key, var_name))
		{
			return env[k];
		}
	}
	return NULL;
}

// Add a variable called "name" with value "value" in the environment
int add_var(char* name, char* value)
{
	for (int k = 0; value[k]; k++)
	{
		// $ sign may lead to infinite recursion the way I coded it so avoid it
		if (value[k]=='$') { errmsg("Please no $ sign as variable value!"); return 0; }
	}
	// I chose to have a limited number of variables because it was easier to code and not so important
	if (indice > MAX_SIZE) { 
		var* vari = search("");
		if (!vari) {errmsg("Plus de place mais flemme d'arranger ça relance le shell\n"); return 0; }
		vari -> key = name;
		vari -> value = value;
	}
	var* newvar = malloc(sizeof(var));
	newvar -> value = value;
	newvar -> key = name;
	env[indice] = newvar;
	indice++;
	return 1;
}

// Change the value of a variable called "name" or create it if it does not exist
int change_var(char* name, char* newvar)
{
	var* vari = search(name);
	if (!vari) return add_var(name, newvar); 
	vari -> value = newvar;
	vari -> value[strlen(newvar)] = 0;
	return 1;
}

// Show the current state
int show_set()
{
	for (int k = 0; k<indice; k++)
	{
		if (env[k] && strlen(env[k] -> key)) printf("%s=%s\n", env[k] -> key, env[k] -> value);
	}
	return 0;
}

// Delete a variable name from the environment
int unset(char* var_name)
{
	for (int k = 0; k<indice; k++)
	{
		if (!strcmp(env[k] -> key, var_name))
		{
			env[k] -> key = "";
			return 0;
		}
	}
	fprintf(stderr, "error: unknown variable name %s\n", var_name);
	return 1;
}

/////////////////////////////////////

/// First the principal functions ///

/////////////////////////////////////

// First do the parser job ie change ~ by the path and $ by the variable value
int handle_special_char(struct cmd *cmd)
{
	int i = 0;
	while (cmd -> args[i]) // For all arguments
	{
		
		char *arg = cmd -> args[i];
		// If we have a ~ representing a path (not a text file !)
		if (arg[0] == '~' && (!(arg[1])|| arg[1] == ' ' || arg[1] == '/'))
		{
			char path[MAX_SIZE];
			sprintf(path, "%s%s", getenv("HOME"), arg + 1);
			path[MAX_SIZE-1] = 0; // In case of buffer overflow
			cmd -> args[i] = path;

		}
		// A $ sign can be anywhere so let's look for one
		int j = 0;
		int n = strlen(arg);
		while (j<n && arg[j]!='$') j++;
		if (j<n)
		{
			int k = j+1;
			// variable name MUST be alphanumerical except predefined ones ie $$ and $?
			while (arg[k] && (isalnum(arg[k]) || arg[k] == '$' || arg[k]=='?')) k++;
			char var_name[1024] = {0};
			strcpy(var_name, arg+j+1);
			var_name[k-j-1] = 0;

			var *tmp = search(var_name);
			char *val = 0;
			if (tmp) val = tmp -> value;
			if (!val)
			{
				// If it does not exists locally it may be in the original shell (as $PATH) so let's look here
				val = getenv(var_name);
				if (!val) { fprintf(stderr, "error: variable name \"%s\" not defined!\n", var_name); return 1; }
			}
			char strC[MAX_SIZE] = {0};
			strncpy(strC, arg, j);
			strC[j] = '\0';
			strcat(strC, val);
			strcat(strC, arg + j + strlen(var_name)+1);
			cmd -> args[i] = strC;
			int j = 0;
			while (arg[j] && arg[j]!='$') j++;
			// If another $ is present it means there is another variable so let's repeat (no $ in the value !)
			if (j<n-1) handle_special_char(cmd);
		}
		i++;
	}
	return 0;
}

//////////////////////////////////////

/// Now some basic functions added ///

//////////////////////////////////////

#include <dirent.h>
int basik_ls (struct cmd *cmd)
{
	struct dirent **namelist;
	int n;
	int i = 1;		
	
	// If no args given
	if (!cmd -> args[1]){
		n = scandir(".", &namelist, NULL, alphasort); 
		for (int i = 0; i<n; i++) {
			if ((namelist[i])->d_name[0]!='.'){
				printf("%s\n", namelist[i]->d_name);
				free(namelist[i]);
			}
		}
		return EXIT_SUCCESS;
	}
	else {
		while (cmd -> args[i]){
			// Just for file name, no wildcard *.tex or else
			n = scandir(cmd -> args[i], &namelist, NULL, alphasort);
			if (n == -1) {
				fprintf(stderr, "error: failed to look into the directory\n");
				return EXIT_FAILURE;
			}
			i++;
		}
	
		// Print the rez line by line (easiest formatting)
		for (int i = 0; i<n; i++) {
			if ((namelist[i])->d_name[0]!='.'){
				printf("%s\n", namelist[i]->d_name);
				free(namelist[i]);
			}
		}
	}
	free(namelist);

	exit(EXIT_SUCCESS);
}



int basik_cat (struct cmd *cmd)
{
	char buffer[MAX_SIZE];
	int n;
	int i = 1;
	// If no arguments
	if (!(cmd -> args[1])){
		n = read(STDIN_FILENO, buffer, MAX_SIZE);
		if (n<0){
			fprintf(stderr, "error : impossible to open current file");
			exit(EXIT_FAILURE);
		}
		buffer[n] = 0;
		printf("%s", buffer);
	}
	// For all files in argument
	while (cmd -> args[i]){

		// If it is about the curent file
		if (!strcmp(cmd -> args[i], "-")){
			n = read(STDIN_FILENO, buffer, MAX_SIZE);
		}
		else {
			int fd = open(cmd -> args[i], O_RDONLY);
			if (fd<0) {
				fprintf(stderr, "error : impossible to open %s\n", cmd -> args[i]);
				exit(EXIT_FAILURE);
			}
		
			n = read(fd, buffer, MAX_SIZE);
			close(fd);
		}
		
		if (n<0){
			fprintf(stderr, "error : impossible to open %s\n", cmd -> args[i]);
			exit(EXIT_FAILURE);
		}
		buffer[n] = 0;
		printf("%s", buffer);
		i++;
	}
	exit(EXIT_SUCCESS);
}

// Strings to know where we work
char lastdir[MAX_SIZE] = {0};
char cwd[MAX_SIZE] = {0};

int basik_cd(struct cmd *cmd) {
	char* arg = cmd -> args[1];
	
	// If no args
    if (arg == NULL) {
        arg = getenv("HOME");
    }

	// If changing to the previous directory
    if (!strcmp(arg, "-")) {
        if (*lastdir == '\0') {
            errmsg("no previous directory\n");
            return 1;
        }
        arg = lastdir;
    }
	// Changing directory
    if (chdir(arg)) {
        fprintf(stderr, "chdir: %s: %s\n", strerror(errno), arg);
        return 1;
    }
    strcpy(lastdir, cwd);
    return 0;
}


// apply_redirects() should modify the file descriptors for standard
// input/output/error (0/1/2) of the current process to the files
// whose names are given in cmd->input/output/error.
// append is like output but the file should be extended rather
// than overwritten.

void apply_redirects (struct cmd *cmd)
{	
	// Just do the thing
	if (cmd -> input)
	{
		int fdin = open(cmd -> input, O_RDONLY);
		if (fdin <0) { fprintf(stderr, "error: I had a problem opening %s\n", cmd -> input); }
		if (dup2(fdin, STDIN_FILENO)<0) { fprintf(stderr, "I had a problem opening %s\n", cmd -> output);}
		if (close(fdin)<0) { fprintf(stderr, "error: I had a problem closing %s\n", cmd -> input); }
	}

	if (cmd -> output)
	{
		int fdout = open(cmd -> output , O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fdout <0) { fprintf(stderr, "error: I had a problem opening %s\n", cmd -> output); }
		if (dup2(fdout, STDOUT_FILENO)<0) { fprintf(stderr, "I had a problem opening %s\n", cmd -> output);}
		if (close(fdout)<0) { fprintf(stderr, "error: I had a problem closing %s\n", cmd -> output); }
	}

	if (cmd -> append)
	{
		int fdapp = open(cmd -> append, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (fdapp <0) { fprintf(stderr, "error: I had a problem opening %s\n", cmd -> append); }
		if (dup2(fdapp, STDOUT_FILENO)<0) { fprintf(stderr, "error: I had a problem opening %s\n", cmd -> output);}
		if (close(fdapp)<0) { fprintf(stderr, "error: I had a problem closing %s\n", cmd -> append); }
	}

	if (cmd -> error)
	{
		int fderr = open(cmd -> error, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fderr <0) { fprintf(stderr, "error: I had a problem opening %s\n", cmd -> error); }
		if (dup2(fderr, STDERR_FILENO)<0) { fprintf(stderr, "error: I had a problem opening %s\n", cmd -> output);}
		if (close(fderr)<0) { fprintf(stderr, "error: I had a problem closing %s\n", cmd -> error); }
	}
	
	return;
}
#define abs(a) (a<0)? -a : a
int gdset(struct cmd *cmd)
{
	return abs(unsetenv(cmd -> args[1]));
}
// Transform all $VAR_NAME into corresponding VAR_VALUE
int apply_var (struct cmd *cmd)
{
	if (cmd -> equal)
	{
		char* var_name = cmd -> args[0];

		if (cmd -> args[1]) fprintf(stderr, "error: too much argument for var definition\n");
		if (!var_name) fprintf(stderr, "error: expected one variable name\n");
		
		for (int j = 0; var_name[j]; j++)
		{
			if (!isalnum(var_name[j]))
			{
				fprintf(stderr, "error: expected alphanumerical name for \"%s\"\n", var_name);
			}
		}

		if (indice >= MAX_SIZE)
		{
			errmsg("Environment buffer is full, sorry not sorry but flemme de coder suppression\n");
		}
		change_var(var_name, cmd -> equal);
		return 1;
	}

	if (cmd -> geq)
	{
		if (setenv(cmd -> args[0], cmd -> geq, 1)<0) errmsg("Impossible to set the variable");
		return 1;
	}
	return 0;
}


// The function execute() takes a command parsed at the command line.
// The structure of the command is explained in output.c.
// Returns the exit code of the command in question.

int handle_C_PLAIN_operator (struct cmd *cmd)
{
	// First some implemented functions that are independant to the shell
	if (!strcmp(cmd -> args[0], "gdset")) return gdset(cmd);
	if (!strcmp(cmd -> args[0], "cd")) return basik_cd(cmd);
	else if (!strcmp(cmd -> args[0], "set")) return show_set();
	else if (!strcmp(cmd -> args[0], "unset")) {
		for (int k = 1; cmd -> args[k]; k++){
			unset(cmd -> args[1]);
		}
		return 0;
	}
	pid_t pid;
	char *name = cmd -> args[0];
	pid = fork();
	if (!pid)
	{
		// Verify if it is sent somwhere
		apply_redirects(cmd);
		// Now some implemented functions that depends on the PATH and loaded files
		if (!strcmp(cmd -> args[0], "ls")) { exit(basik_ls(cmd)); }
		if (!strcmp(cmd -> args[0], "cat")) { exit(basik_cat(cmd)); }

		execvp(name, cmd -> args);
		fprintf(stderr, "error: a problem occured durring \"%s\" handling; please verify syntax!\n", name);
		exit(EXIT_FAILURE);
	}
	int wstatus;


	for (;;) {

		waitpid(pid, &wstatus, 0);

		if (WIFCONTINUED(wstatus)) {
			continue;
		}
		kill(pid, SIGKILL);
		if (WIFSIGNALED(wstatus)) {
			printf("The execution stoped prematurely: %s\n",
					strsignal(WTERMSIG(wstatus)));
		if (WCOREDUMP(wstatus)) {
			printf("It led to a core dump\n");
		}
		} else if (WIFEXITED(wstatus)) {
			return WEXITSTATUS(wstatus);
		}
		break;
	}
	return EXIT_FAILURE;
}


int handle_C_SEQ_operator (struct cmd *cmd)
{
	execute(cmd -> left);
	execute(cmd -> right);
	return EXIT_SUCCESS;
}


int handle_C_AND_operator(struct cmd *cmd)
{
	if (!execute(cmd -> left)) { return execute(cmd -> right); }
	// Doing parser job to continue after a first &&
	struct cmd *tmp = cmd;
	while (tmp -> right -> type == C_AND) { tmp = tmp -> right; }
	// If there is an operator other than && at the right it must be executed.
	// The bash has a left associativity, hence `false && false || echo 2` print 2.
	// Sadly the parser read from the left to the right, it could have been avoided...
	if (tmp -> right -> right) { return execute(tmp -> right -> right); }
	return EXIT_FAILURE;
}

int handle_C_OR_operator(struct cmd *cmd)
{
	if (execute(cmd -> left) > 0) { return execute(cmd -> right); }
	// Fine, I'll do it myself
	struct cmd *tmp = cmd;
	while (tmp -> right -> type == C_OR) { tmp = tmp -> right; }
	if (tmp -> right -> right) { return execute(tmp -> right -> right); }
	return EXIT_FAILURE;
}


int handle_C_VOID_operator(struct cmd *cmd)
{
	
	pid_t pid;
	pid = fork();
	if (!pid)
	{
		// apply_redirect needs to be executed in fork to avoid infinite waiting
		// after dup2(fd, STDOUT_FILENO)...
		apply_redirects(cmd);
		exit(execute(cmd -> left));
	}

	int wstatus;
	for (;;) {

		waitpid(pid, &wstatus, 0);

		if (WIFCONTINUED(wstatus)) {
			continue;
		}
		kill(pid, SIGKILL);
		if (WIFSIGNALED(wstatus)) {
			printf("The execution stoped prematurely: %s\n",
					strsignal(WTERMSIG(wstatus)));
		if (WCOREDUMP(wstatus)) {
			printf("It led to a core dump\n");		}
		} else if (WIFEXITED(wstatus)) {
			return WEXITSTATUS(wstatus);
		}
		break;
	}
	return EXIT_FAILURE;
}


int handle_C_PIPE_operator(struct cmd *cmd)
{
	int pipefd[2];
	int wstatus1, wstatus2;
	pid_t pid1, pid2;

	// We are doing 2 pipes to have both parts executing in parallel

	if ((pipe(pipefd))<0){ errmsg("Cannot pipe\n"); }

	if ((pid1 = fork())<0) { errmsg("Cannot fork 1\n"); }

	if (pid1 == 0) {
		if (close(pipefd[0])<0) errmsg("I had an error closing pipe");
		if (dup2(pipefd[1], STDOUT_FILENO)<0) errmsg("I can't pipe it");
		exit(execute(cmd -> left));

	}
	else{
		if ((pid2 = fork())<0) { errmsg("Cannot fork 2\n"); }
		if (pid2 == 0){
			if (close(pipefd[1])<0) errmsg("I can't close the pipe");
            if (dup2(pipefd[0], STDIN_FILENO)<0) errmsg("I can't pipe it");
            if (close(pipefd[0])<0) errmsg("I can't close it");
            exit(execute(cmd -> right));
		}
		else {
			if(close(pipefd[1])<0) errmsg("I can't close it");
			for (;;) {
				waitpid(pid1, &wstatus1, 0);

				if (WIFCONTINUED(wstatus1)) {
					continue;
				}
				kill(pid1, SIGKILL);
				if (WIFSIGNALED(wstatus1)) {
					fprintf(stderr, "L'exécution de %s s'est arrêté prématurément: %s\n", cmd -> left -> args[0], strsignal(WTERMSIG(wstatus1)));
				if (WCOREDUMP(wstatus1)) { printf("L'exécution a mené à un core dump\n"); }
				return EXIT_FAILURE;
				}
				waitpid(pid2, &wstatus2, 0);

				if (WIFCONTINUED(wstatus2)) {
					continue;
				}
				kill(pid2, SIGKILL);
				if (WIFSIGNALED(wstatus2)) {
					fprintf(stderr, "L'exécution de %s s'est arrêté prématurément: %s\n", cmd -> right -> args[0], strsignal(WTERMSIG(wstatus2)));
				if (WCOREDUMP(wstatus2)) { printf("L'exécution a mené à un core dump\n"); }
				return EXIT_FAILURE;
				}
				break;
			}
			return EXIT_SUCCESS;
		}
	}
	return EXIT_SUCCESS;
}

int execute (struct cmd *cmd)
{
	switch (cmd->type)
	{
	    case C_PLAIN: 
			if (apply_var(cmd)) return 0;
			else if (handle_special_char(cmd)) return 1;
			else return handle_C_PLAIN_operator(cmd);
	    case C_SEQ: return handle_C_SEQ_operator(cmd);
	    case C_AND: return handle_C_AND_operator(cmd);
	    case C_OR: return handle_C_OR_operator(cmd);
	    case C_PIPE: return handle_C_PIPE_operator(cmd);
	    case C_VOID: return handle_C_VOID_operator(cmd);
		default: errmsg("I do not know how to do this, please help me!\n");
		return -1;
	}

	// Just to satisfy the compiler
	// YES it does matter ! Or no maybe I didn't found out yet
	// errmsg("This cannot happen!");
	return -1;
}

void CTRL_C_handler(int signum)
{
	printf("\n%c[32m%s@%s%c[0m:%c[34m%s%c[0m$ ",27,usr,hostname,27,27,cwd,27);
}



int main (int argc, char **argv)
{
	if (!getcwd(cwd, sizeof cwd)) *cwd = '\0';
	strcpy(usr, getUserName());
	char rez_value[5];
	char pid_value[5];
	gethostname(hostname, MAX_SIZE);
	sprintf(pid_value, "%d", getpid());
	add_var("$", pid_value);

	printf("\n%c[33;1mwelcome to %s!%c[0m\n\n", 27, NAME, 27);
	
	char *prompt = malloc(strlen(usr)+strlen(hostname)+strlen(cwd)+23);
	sprintf(prompt, "%c[32m%s@%s%c[0m:%c[34m%s%c[0m$ ",27,usr,hostname,27,27,cwd,27);
	//sprintf(prompt,"%c[32m%s>%c[0m ",27,NAME,27);

	signal(SIGINT, CTRL_C_handler);

	while ("true")
	{
		char *line = readline(prompt);
		if (!line) break;	// user pressed Ctrl+D; quit shell
		if (!*line) continue;	// empty line

		add_history (line);	// add line to history

		struct cmd *cmd = parser(line);
		if (!cmd) continue;	// some parse error occurred; ignore
		//output(cmd,0);	// activate this for debugging
		sprintf(rez_value, "%d", execute(cmd));
		change_var("?",rez_value);
		strcpy(cwd, getcwd(cwd, sizeof cwd));
		strcpy(usr, getUserName());
		sprintf(prompt, "%c[32m%s@%s%c[0m:%c[34m%s%c[0m$ ",27,usr,hostname,27,27,cwd,27);

	}

	printf("\n\n%c[33;1mgoodbye!%c[0m\n\n", 27, 27);
	return 0;
}
