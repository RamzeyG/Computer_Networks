#Makefile
#This file automates the building process and 
#testing Ramzey's Client and Server torrent program

#Ramzey Ghanaim
#Lab3

all : client server

client: src/client.c
	gcc -I -ggdb -Wall -lm src/client.c -o client
	mv client clientBin/

server: src/server.c src/udp_helper.c
	gcc -I -c -ggdb -lm -Wall src/server.c -o server
	mv server serverBin/


clean: serverBin/ clientBin/
	rm -f serverBin/*.o serverBin/server clientBin/*.o clientBin/client
