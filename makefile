CC=gcc
LIBS=-lpthread -lhpdf
	
all: server.c client.c game.c utils.h server.h client.h game.h
	$(CC) server.c $(LIBS) -o server
	$(CC) client.c $(LIBS) -o client
