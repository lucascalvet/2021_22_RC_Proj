CC = gcc
CFLAGS  = -Wall

TARGET = download

SRC = download.c

all: download

debug: CFLAGS += -DDEBUG
debug: download

download: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	$(RM) $(TARGET)
