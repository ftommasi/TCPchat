all:
	gcc -g -pthread -o server server.c
	gcc -g -pthread -o client client.c

