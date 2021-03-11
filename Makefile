CC = gcc
serverclientmake: sproxy.c cproxy.c
	$(CC) -o sproxy sproxy.c
	$(CC) -o cproxy cproxy.c