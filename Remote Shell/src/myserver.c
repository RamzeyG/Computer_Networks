/*    Lab 1
*
*	  myserver.c
*     Ramzey Ghanaim
* 
* This program is a server that connects
* with the client program in this directory.
* The server takes in shell commands from
* the client, executes them, and sends the
* output back to the client
*
*/

#define MAX 50//128000
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include  <signal.h> //CTRL C handeling
#include <stdio.h>
#include <stdlib.h>

//used to detect ctrl c to exit the program
void CTRLc(int); 
int getSize(int strLength, int size);

//result and connected are public so they can be freed
//in the CTRLc() function
char *result;
int connected;
int main(int argc, char* argv[]){

	//if one argument assume it is a port
	if(argc !=2){
		printf("Usage: <Port>\n");
		exit(1);
	}

	//tell program to listen for ctrl+C
	signal(SIGINT, CTRLc);

	//used to execute commnands with popen()
	FILE *fileStuff;
	
	//socket info about the client connecting to this server
	struct sockaddr_in client;

	//socket info about this server
	struct sockaddr_in server;
	
	//crate a socket
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	//check for errors
	if(socketFD <0){
		printf("Error: Can't make a socket\n");
		exit(1);
	}

	
	//set up attributes
	server.sin_family = AF_INET; //set connection to TCP
	server.sin_port = htons(atoi(argv[1])); //assigns port number
	server.sin_addr.s_addr = inet_addr("127.0.0.1");//sets addr to any interface
	
	//bind server information to socket
	int x = bind(socketFD, (struct sockaddr *) &server, sizeof(server));
	
	//check for errors
	if(x<0){
		printf("Can't bind\n");
		exit(1);
	}
	
	//start listening for clients, on the socket we made
	//and listen for up to 5 clients at a time
	listen(socketFD,5);
	
	int addrSize = sizeof(struct sockaddr_in);
	char command[MAX+1]; //command given by client
	char cmd[MAX+1];
	char buf[MAX+1];//
	
	int strLength, temp; 
	int done = 0;
	result = (char *)malloc(sizeof(char *) * (MAX+1)); 
	memset(command, 0, sizeof(command));
	char * errorCMD = "ERROR, Invalid Command\n";
	char * failed = "ERROR: failed to execute command\n";
	printf("waiting for clients...\n");
	while((connected = accept(socketFD,
							   (struct sockaddr *)&client,
							   (socklen_t*)&addrSize)) >0)
	{ //while traffic is coming
		printf("Welcome client, you've been expected!\n");
		//read in command from client
		begin:
		while(recv(connected,command, sizeof(command),0) > 0) {
			
			//look for @ end char of the string
			int len = strnlen(command,MAX+1);
			if(command[len-1] != '@'){
				write(connected, "ERROR Didn't get entire string\n",31);
				goto begin;
			}
			
			//extract the command without the end char
			strncpy(cmd,command,len-1);
			//printf("COMMAND: %s",cmd);
			fileStuff = popen(cmd,"r"); //executes command
			
			//if error executing popen, tell user
			//This is NOT if command is not valid
			if(fileStuff == NULL) { 
				int str = strlen(failed);
				//size = getSize(str, size);
				write(connected, failed, str); //can't do process
			} 
		
				//clear the command and buffer
				memset(command, 0, MAX+1);
				memset(cmd,0,MAX+1);
				memset(buf, 0, MAX+1);
				memset(result, 0, MAX+1);
				
			//read until done
			done =0;
			//isize of fgets result for the Next string
			int N;
			//next string to send
			char Next[MAX+1];
			fgets(Next, MAX, fileStuff);
			
			//continously send data until done
			//by copying the next line being sent into the 
			//sending buffer string (buf)
			while(done ==0){
				
				//copy next string into the sending buffer (buf)
				strcpy(buf, Next);
				
				//get the length of the sending buffer
				strLength= strnlen(buf, MAX);
				
				//reset the Next buffer so we can refill it
				memset(Next,0,MAX+1);
				
				//if there is more data to send && we maxed out the size...
				if((N = (fgets(Next,MAX,fileStuff))!=0)  && strLength == MAX){
					
					//...write the current buffer...
					if(write(connected,buf,strLength)<=0){
						printf("Client broke during send\n");
					}
					
					//...and tell the client i'm not done yet!
					write(connected,"!",1); 
				}
				
				//if there's no more stuff to get...
				else if(N== 0){
					
					//...send the last string and set done flag to 1
					write(connected,buf,strLength);
					done = 1;
				}
				
				//if we are not done, simply write the current buffer
				else if(Next != 0){
					write(connected,buf,strLength);
				}
				
				//reset the buffer
				memset(buf, 0, MAX+1);
			}
				
			
			
			//close the file
			temp = pclose(fileStuff);

			//check to see if user entered invalid command and
			//send the message back to the client
			if(temp != 0 ){//&& strlen(result) == 0){ 
				int len = strnlen(errorCMD, MAX);
				write(connected, errorCMD, len);
			}
			
			//lastly write the done char
			int g = write(connected,"@", 1);
			if(g <=0 )
			{
				printf("Can't write DONE\n");
			}
									
		} // exit "recv()" while loop
		
	} // exit "connect()" while loop
	
	//free result to avoid mem leaks
	free(result);
	close(connected);//close connect to avoid mem leaks
	return 0;
}


//when CTRL C is clicked, ask
//user if they want to quit
void  CTRLc(int sig)
{
     char  c;

     signal(sig, SIG_IGN);
     printf("\nDo you really want to quit the server? [y/n]: ");
     c = getchar();
     if (c == 'y' || c == 'Y'){
         exit(0);
		  	//free result to avoid mem leaks
		free(result);
		close(connected);//close connect to avoid mem leaks
	 }
     else
          signal(SIGINT, CTRLc);
     getchar(); // Get new line character
}
