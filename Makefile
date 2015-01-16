all: server client
	chmod 777 ./bin/* 
client.o:
	gcc -c -g -Wall client.c -o ./obj/client.o

server.o:
	gcc -c -g -Wall server.c -o ./obj/server.o




client: client.o
	gcc ./obj/client.o -lpthread -o ./bin/client

server: server.o
	gcc ./obj/server.o -lpthread -o ./bin/server



exec_server:
	clear
	./bin/server
exec_client:
	clear
	./bin/client

clean: 
	rm ./bin/* ./obj/*