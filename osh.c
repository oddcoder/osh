#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
char command[512];
char *shell;
char *args[265];
char fullpath[100];
bool background;
char *command_start = command;
char *get_next_command() {
	write(1, shell, strlen(shell));
	int x;
	while ((x = read(0, command, 512)) == 512) {
		write(2, "Error: Very long Command", strlen("Error: Very long Command"));
		write(1, shell, strlen(shell));

	};
	command[x] = 0;
}
bool process_commands() {
	//right trim
	background = false;
	size_t end = strlen(command_start) - 1, start = 0, argc = 0, i = 0;
	while (command_start[end] == ' ' || command_start[end] == '\n') {
		command_start[end] = 0;
		end--;
	}
	// left trim
	while (command_start[start] == ' ' || command_start[start] == '\n') {
		command_start[start] = 0;
		start++;
	}
	if (command_start > command + end) {
		command_start = command;
		return false;
	}
	while(i <= end) {
		if (command_start[i] == ' ') {
			args[argc++] = command_start + start;
			args[argc] = 0;
			while (command_start[i] == ' ') {
				command_start[i] = 0;
				i++;
			}
			start = i; 
		}
		if (command_start[i] == '&') {
			if(argc == 0) {
				write(2, "unexpected &\n", strlen("unexpected &\n"));
				return false;
			}
			if (command_start[i-1] != 0) {
				command_start[i] = 0;
				args[argc++] = command_start + start;
				args[argc] = 0;
			}
			if (i == end ) {
				background = true;
			}
			command_start = command_start + start + 1;
			return true;
		}
		i++;
	}
	args[argc++] = command_start + start;
	args[argc] = 0;
	command_start = command_start + end + 1;
	return true;
}
bool resolve_path() {
	char *path = getenv("PATH");
	size_t l = strlen(path);
	char *start = path;
	char *end = path;
	struct stat sb;
	if (strstr(args[0], "/")) {
		if (stat(args[0], &sb) == 0 && sb.st_mode & S_IXUSR) {
			return true;
		} else {
			write(2, "Command Not Found\n", strlen("Command Not Found\n"));
			return false;
		}
	}
	while (end != path + l) {
		if (*end == ':') {
			strncpy(fullpath,start, end - start);
			fullpath[end-start] = 0;
			strcat(fullpath, "/");
			strcat(fullpath, args[0]);
			if (stat(fullpath, &sb) == 0 && sb.st_mode & S_IXUSR) {
				args[0] = fullpath;
				return true;
			}
			start = end + 1;
		}
		end ++;
	}
	write(2, "Command Not Found\n", strlen("Command Not Found\n"));
	return false;
}
int main() {
	if (getuid()==0) {
		char *message = "#1) Respect the privacy of others.\n"
    				"#2) Think before you type.\n"
    				"#3) With great power comes great responsibility.\n";
		write(1, message, strlen(message));
		shell = "osh-0.1# ";
	} else {
		shell = "osh-0.1$ ";
	}
	while(true) {
		get_next_command();
		while (process_commands()) {
			if (!strcmp(args[0], "exit")) {
				exit(0);
			}else if (!strcmp(args[0], "cd")) {
				if (args[1] == NULL) {
					args[1] = getenv("HOME");
				}
				chdir(args[1]);
				continue;
			}
			if (!resolve_path()) {
				while (process_commands());
				break;
			}
			pid_t p = fork();
			if (p == 0) {
				if (background) {
					close(0);
					close(1);
					close(2);
				}
				execve(args[0], args, NULL);
			} else {
				if (!background) {
					waitpid(p, NULL, 0);
				}
			}
		}
	}
}
