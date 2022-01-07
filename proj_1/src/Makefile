CC = gcc
CFLAGS  = -Wall

W_TARGET = wnc
R_TARGET = rnc

COMMON_SRC = link_layer.c app.c
COMMON_H = link_layer.h app.h

all: writer reader

writer: writenoncanonical.c $(COMMON_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -o $(W_TARGET) writenoncanonical.c $(COMMON_SRC)

reader: readnoncanonical.c $(COMMON_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -o $(R_TARGET) readnoncanonical.c $(COMMON_SRC)

clean:
	$(RM) $(W_TARGET) $(R_TARGET)
