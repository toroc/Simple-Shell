
all : simple_shell

simple_shell: 
	gcc -Wall -g simple_shell.c -o simple_shell



clean:
	rm -f *.o simple_shell
