all: clean server_main.o
	gcc -l pthread -o server_main.out server_main.o
server_main.o:
	gcc -c server_main.c
clean:
	rm -f *.o *~

