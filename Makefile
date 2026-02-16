CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
LDFLAGS = -lcurl

TARGET = nxpm
SRC = nxpm.c cJSON.c

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

install:
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/
	mkdir -p /var/lib/nxpm
	mkdir -p /var/cache/nxpm/sources

uninstall:
	rm -f /usr/local/bin/$(TARGET)
