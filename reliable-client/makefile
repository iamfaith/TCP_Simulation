client: client.o
	gcc -o client client.o
	
client.o: client.c
	gcc -c client.c

clientM: client_multiplexing.c
    gcc -o client_m client_multiplexing.c

clean:
	rm -f client *.o

