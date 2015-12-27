CC=gcc
CFLAGS=-lpthread
DBFLAGS=-g -DDEBUG
luemiu: dump.o miu.o main.o
	$(CC) -o luemiu $(CFLAGS) $(DBFLAGS) main.o miu.o dump.o

main.o: main.c main.h
	$(CC) $(DBFLAGS) -c main.c
miu.o: miu.c main.h
	$(CC) $(DBFLAGS) -c miu.c
dump.o: dump.c main.h
	$(CC) $(DBFLAGS) -c dump.c

clean:
	-rm -rf *.o
