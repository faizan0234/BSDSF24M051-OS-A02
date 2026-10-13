
CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = bin/ls
SOURCE = src/ls-v1.1.0.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
