/*    Lab 1
*
*	  myclient.c
*     Ramzey Ghanaim
* 
* This program is a client that takes in shell
* commands and sends the command to a server.
* The server will execute the program and return
* the result back to the client where the client
* will simply display the result
*
*/

#define MAX 50//128000
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char* argv[]){
	
	//if one argumet assume it is a port
	if(argc !=3){
		printf("Usage: <Address> <Port>\n");
		exit(1);
	}

	//create a socket with TCP and 2 way thorughput with no flags
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	// Error out if socket cannot be made
	if(socketFD <0){ 
		printf("ERROR: Incorrect Socket\n");
		exit(1);
	}
	
	//create a server address
	struct sockaddr_in serverAddr; 

	//set server family to TCP connection
	serverAddr.sin_family = AF_INET;

	//asign port to the server based on user input
	int temp = serverAddr.sin_port = htons(atoi(argv[2]));

	//error out if not set up correctly
	if(temp<0){
		printf("ERROR: inet_pton\n");
		exit(1);
	}


	// inet_pton(Internetwork , src addr (strig form) , dest)
	serverAddr.sin_addr.s_addr=inet_addr(argv[1]);
	
	//connect to the server at the socket we made earlier
	int con = connect(socketFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	
	//if error, tell user
	if(con<0){
		printf("failed to connect\n");
		exit(1);
	}

	char input[MAX+1]; //user command
	char result[MAX+1];//final result from server
	int g; //increment or that looks for done chars	
	
	//primary loop of the progam
	while(1){
		begin:
		//prompt
		memset(input,0,MAX);
		memset(result,0,MAX+1);
		printf("\nclient $ ");

		//get user input
		fgets(input, MAX, stdin);
		
		//if the user typed "exit", quit the client
		//Don't send to server because a client
		//should not be able to kill a server
		//in many applications
		if (strcmp(input, "exit\n") == 0){
			close(socketFD);
			break;
		}	

		//attach end of message char
		strncat(input, "@",MAX+1);
		
		//send the command (user input to the server)
		//and check for error
		if (send(socketFD, &input, strlen(input), 0) < 0)
			printf("%s\n", "unable to send");
		
		
		//reading from the server
		while(recv(socketFD,result,sizeof(result),0) > 0){
			int len = strnlen(result,MAX+1);
			
			//increment through the string that was received
			g = 0;
				while(g<len){
					if(result[g] == '@' && g == len-1){ //check for @ at end of string
					
					// AT-AT case
					// we got an  @ @ at the end of our string, on of them is printed
					//one of them is the end of string since the very last index in 
					//the buffer is reserved for end of message (@) or expect more messages (!)
						if(g == MAX-1 && result[g+1] =='@'){
							printf("%c",result[g]);
							goto begin;
						}
						

					//AT-! case (we ended with an @ ,but not done getting data
						else if(g == MAX-1 && result[g+1] == '!'){
							printf("%c",result[g]);
							goto next;
						}

						//Normal case Go to beginning
							goto begin;
					}
					
					//otherwise we have a normal char, so print it
					printf("%c",result[g]);	
					g++;
					

					
				}//while g <len
			next:
		//clear the server's response array
			memset(result, 0, MAX+1);
		}//end while(revc())

	}
	//close the socket to prevent mem leaks
	close(socketFD);
return 0;
}
