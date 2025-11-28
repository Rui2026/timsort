CC = gcc
CFLAGS = -O3 -march=native

all: timsort

timsort: src/timsort.c src/main.c
	$(CC) $(CFLAGS) -o timsort src/timsort.c src/main.c

run: timsort
	./timsort

clean:
	rm -f timsort
