#Makefile
#
#Ramzey Ghanaim
#Lab5

all : ftpclient ftpserver

ftpclient: src/ftpclient.c
	gcc -pthread -I -ggdb -Wall src/ftpclient.c -o ftpclient
	mv ftpclient clientBin/
		
ftpserver: src/ftpserver.c
	gcc -pthread -I -ggdb -Wall src/ftpserver.c -o ftpserver
	mv ftpserver serverBin/	
	

clean: serverBin/ clientBin/
	rm -f serverBin/*.o serverBin/ftpserver clientBin/*.o clientBin/ftpclient
