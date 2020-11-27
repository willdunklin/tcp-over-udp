CC=gcc

default:
	$(CC) server.c -o server.o
	$(CC) client.c -o client.o

clean:
	rm *.o