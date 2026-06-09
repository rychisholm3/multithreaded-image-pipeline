CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11 -Isrc
LDFLAGS = -lpthread -lm

SRCS    = src/main.c src/image.c src/filters.c src/thread_pool.c src/pipeline.c
OBJS    = $(patsubst src/%.c, build/%.o, $(SRCS))
TARGET  = build/pipeline

.PHONY: all clean run run1 run8

all: build $(TARGET)

build:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET) 4

run1: all
	@echo "=== Single-threaded ==="
	./$(TARGET) 1

run8: all
	@echo "=== 8-threaded ==="
	./$(TARGET) 8

clean:
	rm -rf build input output
