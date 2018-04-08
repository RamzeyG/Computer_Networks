#Makefile
#This file automates the building process and 
#testing Ramzey's Client and Server torrent program

#Ramzey Ghanaim
#Lab2

all : myclient myserver

myclient: src/myclient.c
	gcc -pthread -I -ggdb -Wall src/myclient.c -o myclient
	mv myclient bin/
		
myserver: src/myserver.c
	gcc -I -ggdb -Wall src/myserver.c -o myserver
	mv myserver bin/	
	

clean: bin/
	rm -f serverBin/*.o bin/myserver 
