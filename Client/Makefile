all: clean client_main.o
	gcc -l pthread -o client_main.out client_main.o
client_main.o:
	gcc -c client_main.c
clean:
	rm -f *.o *~


