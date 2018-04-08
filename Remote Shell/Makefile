#Makefile
#This file automates the building process and 
#testing Ramzey's Client and Server shell

#Ramzey Ghanaim
#Lab1

all : myclient myserver

myclient: src/myclient.c
	gcc -Wall -I src src/myclient.c -o myclient
	mv myclient bin/
		
myserver: src/myserver.c
	gcc -Wall -I src src/myserver.c -o myserver
	mv myserver bin/	
	
test: bin/myclient bin/myserver /bin/test.py
	 bin/python3 test.py
	
runserver: bin/myserver
	bin/myserver 1234
	
runclient: bin/myclient
	bin/myclient 127.0.0.1 1234

infinite: src/infinite.c
	gcc -I src src/infinite.c -o infinitetest
	mv infinitetest bin/

clean: bin/
	rm -f bin/*.o bin/myserver bin/myclient bin/infinitetest 
