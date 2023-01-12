PHONY: all

all: server.o client.o
	gcc server.o -o server -lpthread
	gcc client.o -o client -lpthread
server.o: server.c
	gcc server.c -c -lpthread
client.o: client.c
	gcc client.c -c -lpthread

