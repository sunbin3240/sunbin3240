# Makefile for i2c demo
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2

TARGET := test
SRCS := test.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)