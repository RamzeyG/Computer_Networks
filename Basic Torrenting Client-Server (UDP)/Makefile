#Makefile
#This file automates the building process and 
#testing Ramzey's Client and Server torrent program

#Ramzey Ghanaim
#Lab3

all : myclient myserver

myclient: src/myclient.c
	gcc -pthread -I -ggdb -Wall src/myclient.c -o myclient
	mv myclient clientBin/
		
myserver: src/myserver.c
	gcc -I -ggdb -Wall src/myserver.c -o myserver
	mv myserver serverBin/	
	

clean: serverBin/ clientBin/
	rm -f serverBin/*.o serverBin/myserver clientBin/*.o clientBin/myclient
