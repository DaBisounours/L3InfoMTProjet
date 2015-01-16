all: client
	chmod 700 ./bin/* 
client.o:
	gcc -c -g -Wall client.c -o ./obj/client.o

client: client.o
	gcc ./obj/client.o -lpthread -o ./bin/client

exec_client:
	clear
	./bin/client 1 -t 20 -l 20

clean: 
	rm ./bin/client ./obj/*