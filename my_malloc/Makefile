all: test.c mymalloc.o
	gcc mymalloc.o test.c

mymalloc.o: mymalloc.c mymalloc.h
	gcc -c mymalloc.c

clean: 
	rm *.o; rm a.out
