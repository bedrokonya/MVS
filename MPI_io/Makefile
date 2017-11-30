CC = mpicc
CFLAGS = 
all: run
run: main.o
	$(CC) $(CFLAGS) main.o -o run
main.o: main.c
	$(CC) $(CFLAGS) main.c -c
clean:
	rm -f run main.o data.bin stats.txt
