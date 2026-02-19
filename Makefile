.PHONY: all clean

all: ACS

ACS: acs.o queue.o customer.o
	gcc -Wall -pthread acs.o queue.o customer.o -o ACS

acs.o: acs.c queue.h customer.h
	gcc -Wall -pthread -c acs.c

queue.o: queue.c queue.h customer.h
	gcc -Wall -pthread -c queue.c

customer.o: customer.c customer.h
	gcc -Wall -pthread -c customer.c

clean:
	-rm -f *.o ACS