CC=gcc
LIBS=-lpthread -lhpdf
	
all: server.c client.c game.c utils.c 
	$(CC) server.c $(LIBS) -o server
	$(CC) client.c $(LIBS) -o client
