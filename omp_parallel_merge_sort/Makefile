CC = gcc
CFLAGS = -fopenmp
all: run
run: parallel_merge_sort.o
	$(CC) $(CFLAGS) parallel_merge_sort.o -o run
parallel_merge_sort.o: parallel_merge_sort.c
	$(CC) $(CFLAGS) -c parallel_merge_sort.c
clean:
	rm -f run parallel_merge_sort.o
