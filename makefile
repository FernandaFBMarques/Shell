all: shell.c
		gcc shell.c -g -o shell
clean:
		rm shell