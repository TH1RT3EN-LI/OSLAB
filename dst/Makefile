CC := gcc
CFLAGS := -Wall -O2 -std=c99

TARGET := fibo

all: $(TARGET)

$(TARGET): fibo.c
	$(CC) $(CFLAGS) -o $(TARGET) fibo.c

clean:
	rm -f $(TARGET)
