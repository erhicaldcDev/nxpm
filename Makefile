CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -D_GNU_SOURCE
LDFLAGS = -lcurl -lcjson

TARGET = nxpm
SRC = nxpm.c

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/
	mkdir -p /var/lib/nxpm
	mkdir -p /var/cache/nxpm/sources

uninstall:
	rm -f /usr/local/bin/$(TARGET)