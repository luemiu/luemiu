CC=gcc
CFLAGS=-lpthread
DBFLAGS=-g -DDEBUG
OBJ=dump.o miu.o main.o
ALL:$(OBJ)
	$(CC) $(DBFLAGS) -c $(foreach name, $(shell echo $(OBJ) | sed s/.o//g), $(name).c)
	$(CC) -o luemiu $(CFLAGS) $(DBFLAGS) $(OBJ)
	-rm -f *.o
