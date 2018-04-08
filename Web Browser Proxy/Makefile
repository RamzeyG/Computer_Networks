#Makefile
#This file automates the building process and 
#testing Ramzey's Client and Server shell

#Ramzey Ghanaim
#Lab4

all : proxy

proxy: src/proxy.c
	gcc -ggdb -Wall -I src src/proxy.c -o proxy
	mv proxy bin/
		
run: bin/proxy
	bin/./proxy 8080 bin/forbidden.txt

clean: bin/
	rm -f bin/*.o bin/proxy rm access.log 
