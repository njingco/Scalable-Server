CC = gcc -Wall -g
CLIB = -lpthread -lrt -lgmp

all: server client 

server: server.o 
	$(CC) -o server server.o $(CLIB)

client: client.o semaphore.o
	$(CC) -o client client.o semaphore.o $(CLIB)

clean:
	rm -f *.o core.* server client

server.o:
	$(CC) -c server.c

client.o:
	$(CC) -c client.c

semaphore.o:
	$(CC) -c semaphore.c
	 