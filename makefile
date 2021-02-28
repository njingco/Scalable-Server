CC = gcc -Wall -g
CLIB = -lpthread -lrt -lgmp

all: server client 

server: server.o 
	$(CC) -o server server.o $(CLIB)

client: client.o 
	$(CC) -o client client.o $(CLIB)

clean:
	rm -f *.o core.* server client serverLog.csv

server.o:
	$(CC) -c server.c

client.o:
	$(CC) -c client.c
