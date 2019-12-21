#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <cstring>

int main(int argc, char** argv)
{
	int fd, count;
	char buffer[32000];

	if(argc == 1){ //if there is no file, read from standard input.
		while((count = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0){
			write(STDOUT_FILENO,buffer,count);
		}
	}
	for(int i = 1; i<argc; i++){ //loop each file after the command dog.
		if(strcmp(argv[i], "-") == 0){ //if is a dash, read from stdin.
			while((count = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0){
			write(STDOUT_FILENO,buffer,count);
			}
			continue;
		}
		fd = open(argv[i], O_RDONLY);//find the file.
		if(fd == -1){ //if the file doesn't exist, write to stdout.
			write(STDERR_FILENO,"dog: ", sizeof("dog: "));
			write(STDERR_FILENO,": ", sizeof(": "));
			write(STDERR_FILENO,"No such file or directory \n", sizeof("No such file or directory \n"));
			continue;
		}
		while ((count = read(fd, buffer, sizeof(buffer))) > 0){ //file exist, write to stdout.
			write(STDOUT_FILENO, buffer, count);
		}
	}
	close(fd);
}