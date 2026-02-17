CC = gcc
CFLAGS = -Wall -Wextra -I./src -D_GNU_SOURCE
LDFLAGS = -lcurl

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = nxpm

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/nxpm
