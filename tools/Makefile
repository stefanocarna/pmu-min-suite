CC=gcc 
CFLAGS=-I../include

all: wrapper reader mreader

wrapper: wrapper.c
	$(CC) -o wrapper wrapper.c

reader: reader.c
	$(CC) -o reader reader.c rawr.c seqr.c mmapr.c

clean:
	rm -f wrapper reader mreader