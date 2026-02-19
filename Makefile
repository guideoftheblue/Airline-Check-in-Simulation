.PHONY: all clean

all: ACS

ACS: ACS.o queue.o customer.o
	gcc -Wall -pthread ACS.o queue.o customer.o -o ACS

ACS.o: ACS.c queue.h customer.h
	gcc -Wall -pthread -c ACS.c

queue.o: queue.c queue.h customer.h
	gcc -Wall -pthread -c queue.c

customer.o: customer.c customer.h
	gcc -Wall -pthread -c customer.c

clean:
	-rm -f *.o ACS