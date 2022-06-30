all:
	gcc -Wall -c common.c
	gcc -Wall equipment.c common.o -lpthread -o equipment
	gcc -Wall server.c common.o -lpthread -o server

clean:
	rm common.o equipment server

