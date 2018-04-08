//             Ramzey Ghanaim
//                ftpclient.c
//                  Final Lab
//     
//
// 
//
#define LIST 1000
#define MAX 1000000
#define IPMAX 50
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include  <signal.h> //CTRL C handeling

int checkArgs(int *args, char *argv[]);
int cmdConn(int port);
int sendRequests(int cmdSock, char buffer[MAX]);
void getLSData(char buffer[MAX], int cmdSock);
int dataConn();
//used to detect ctrl c to exit the program
void CTRLc(int); 

//socket info about the client connecting to this server
struct sockaddr_in client;

//socket info about this server
struct sockaddr_in server;

int  dataPort = 6462;
char ip[IPMAX];
char sendBuff[MAX];
int exitFlag;
int socketFD;
int main(int argc, char* argv[]){
	int port = checkArgs(&argc,argv);
	socketFD  = cmdConn(port);
	char buffer[MAX];
	int exitFlag = 0;
	//tell program to listen for ctrl+C
	signal(SIGINT, CTRLc);
	
	strncpy(buffer,"PORT 127,0,0,1,25,62",MAX);// port 6462
	if(send(socketFD,buffer,strnlen(buffer,MAX),0) < 0){
		perror("could not send PORT");
	}
	memset(buffer,0,MAX);
	if(recv(socketFD,buffer,MAX,0) < 0){
		perror("can't recv PORT ack");
	}
	if(strcmp(buffer,"200 Command OK.") == 0){
		while(!exitFlag){
			printf("ftp> ");
			exitFlag = sendRequests(socketFD, buffer);
		}
	}else{
		//printf("I GOT: [%s]\n",buffer);
	//	printf("NO ACK for PORT\n");
		exit(1);
	}
	printf("Exiting program\n");
	return 0;
}


// This function checks for valid commands and executes
// the commands if they are valid by sending requests
/// to the ftp server.
int sendRequests(int cmdSock, char buffer[MAX]){
	memset(buffer,0,MAX);
	fgets(buffer,MAX,stdin);
	//printf("YOU TYPED IN:[%s]\n",buffer);
	if(strncmp(buffer, "quit",4) == 0){
		if(send(cmdSock,"QUIT",4,0) < 0){
			perror("Can't send QUIT");
			//exit(1);
		}
		memset(buffer,0,MAX);
		if(recv(cmdSock,buffer,MAX,0) < 0){
			perror("Can't recieve Quit ack");
			//exit(1);
		}
		if(strncmp(buffer,"221 Goodbye.",strlen(buffer)) == 0){
			printf("About to close connection\n");
			close(cmdSock);
			//exitFlag = 1
			return 1;
		}
	}
	//if user typed ls
	else if(strncmp(buffer,"ls",2) == 0){
		char *token;
		if(strncmp(buffer,"ls\n",3)==0 || strncmp(buffer,"ls \n",4)==0 ){
			//printf("501 Syntax error (invalid arguments).\n");
			strcpy(buffer,"LIST");
		}else if(strstr(buffer," ") ==0){
			printf("Syntax error (unrecognized command).\n");
			return 0;
		}
		else{
			//char *space;
			strncpy(sendBuff,buffer,MAX);
			token = strtok(sendBuff," ");
			if(token == NULL){
				printf("500 Syntax Error (unrecognized command)\n");
				return 0;
			}
			strcpy(buffer,"LIST ");
			token = strtok(NULL,"\n");
			strcat(buffer, token);
		}
		//printf("ABOUT TO SEND: [%s]\n",buffer);
		
		if(send(cmdSock,buffer,MAX,0)<0){
			perror("Failed to send ls command");
			exit(1);
		}
		//memset(buffer,0,MAX);
		//printf("should have gotten 200 ok: [%s]\n",buffer);
		
			//get data for LS command
			getLSData(buffer,cmdSock);
		
		
		
	} 
	//download file from ftp server if user enters get
	//command
	else if(strncmp(buffer,"get",3) == 0){
		char *token;
		if(strlen(buffer)<=4){
			printf("Error. need paramters\n");
			return 0;
		}
		if(strstr(buffer," ") ==0){
			printf("Syntax error (unrecognized command).\n");
			return 0;
		}
		strncpy(sendBuff,buffer,MAX);
		token = strtok(sendBuff," ");
		strcpy(buffer,"RETR ");
		token = strtok(NULL,"\n");
		char *fileName = token;
		printf("FILE NAME IS: [%s]\n",fileName);
		strcat(buffer, token);
		printf("AOUT TO SEND [%s]\n",buffer);
		if(send(cmdSock,buffer,MAX,0)<0){
			perror("Failed to send ls command");
			exit(1);
		}
		
		
		memset(buffer,0,MAX);
		if(recv(cmdSock,buffer,MAX,0) < 0){
			perror("Failed to recv ls ack");
			exit(1);
		}
		//printf("should have gotten 200 ok: [%s]\n",buffer);
		if(strcmp(buffer,"200 Command OK.") == 0){
			printf("%s\n",buffer);
			int dataSock = dataConn();
			//get file size
			memset(buffer,0,MAX);
			if(recv(dataSock,buffer,MAX,0) <0){
				perror("CAN't get file size");
			}
			else{
				printf("About to send ack\n");
				send(dataSock,"ACK",MAX,0);
				//printf("SENT ACK\n");
				//printf("BUFFER IS:[%s]\n",buffer);
				if(strncmp(buffer,"501 Syntax error (invalid arguments).",MAX)==0){
					printf("Invalid arguments\n");
				}
				else{
					int fileSize = atoi(buffer);
					//printf("FILE SIZE IS: %i\n",fileSize);					
					int remain_data = fileSize;
					FILE * file = fopen(fileName,"w");
					int got_data;
					int total = 0;
					//sleep(1);
					struct timeval tv;
					tv.tv_sec = 2;
					tv.tv_usec = 0;
					setsockopt(dataSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
					while(remain_data >0){
						memset(buffer,0,MAX);
						//printf("ABOUT TO GET DATA\n");
						if((got_data = recv(dataSock,buffer,MAX,0) )<0){
							//printf("       DATA IS: \n[%d]\n",got_data);
							//perror("got nothing\n");
							break;
						}
						//
						if(total == fileSize){
							printf("HERE\n");
							break;
						} 
						fwrite(buffer,1,strnlen(buffer,MAX),file);
						remain_data-=got_data;
						//printf("got data is: %i len is %i\n",got_data,(int) strlen(buffer));
						total +=got_data;
						
					}
					//printf("GOT %i\n",total);
					fclose(file);
				}
			}
			close(dataSock);
		}else{
			printf("Did not get 200 OK (ls)\n");
		}
		
	}
	//upload a file if user enters put command
	 else if(strncmp(buffer,"put",3) == 0){
		 char *token;
		if(strlen(buffer)<=4){
			printf("Error. need paramters\n");
			return 0;
		}
		strncpy(sendBuff,buffer,MAX);
		token = strtok(sendBuff," ");
		if(token == 0){
			printf("Syntax error\n");
			return 0;
		}
		strcpy(buffer,"STOR ");
		token = strtok(NULL,"\n");
		char *fileName = token;
		//printf("FILE NAME IS: [%s]\n",fileName);
		FILE * file;
		if((file = fopen(fileName,"r")) == NULL){
			printf("File does not exist\n");
			return 0;
		}
		strcat(buffer, token);
		//printf("AOUT TO SEND [%s]\n",buffer);
		if(send(cmdSock,buffer,MAX,0)<0){
			perror("Failed to send ls command");
			exit(1);
		}
		
		
		memset(buffer,0,MAX);
		if(recv(cmdSock,buffer,MAX,0) < 0){
			perror("Failed to recv ls ack");
			//exit(1);
		}
		if(strcmp(buffer,"200 Command OK.") != 0){
			printf("DID not get ok\n");
			return 0;
		}
		
		int dataSock = dataConn();
		//get to end of file 
		fseek(file,0,SEEK_END);
		
		//pointer location is now the 
		//size of the file
		int fileSize = ftell(file);
		
		//put pointer back to beginning
		rewind(file);

		//get message ready to send
		sprintf(buffer,"%d",fileSize);
		
		//printf("file size: %i bytes\n", fileSize);
		//send file size
		if(send(dataSock,buffer,strlen(buffer),0) < 0){
			printf("can't send file length\n");
		}
		int sent;
		sleep(1);
		int total= 0;
		while(1){
			memset(buffer,0,MAX);
			if((sent = fread(buffer, 1, MAX, file))==0){
				//printf("breaking sent\n");
				break;
			}
			
			//printf("sent is: %i buffer len is %i\n",sent,(int) strlen(buffer));
			
			//printf("About to send[%s]\n",buffer);
			fileSize -=sent;
			total+=sent;
			send(dataSock,buffer,MAX,0);
			if(fileSize ==0){
				//printf("breaking filseise = 0\n");
				break;
			}
		}
		//send(cmdSock,"226 Closing data connection.",MAX,0);
		//printf("Sent: %i\n",total);
		fclose(file);
		close(dataSock);
		
	 }
	 else{
		 printf("Command not supported\n");
	 }
	
	return 0;
}

void getLSData(char buffer[MAX], int cmdSock){
	
	int dataSock = dataConn();
	int size;
	do{
		memset(buffer,0,MAX);
		if((size = recv(dataSock,buffer,MAX,0))<0){
			perror("can't get LS data");
			exit(1);
		}
		//memset(buffer,0,MAX);
		if(strstr(buffer,"200 Command OK.") != 0){
			int l = strlen(buffer)-15;
			//if(strcmp(buffer,"501 Syntax error (uncrecognized command).") !=0
			if(strcmp(buffer,"501 Syntax error (invalid arguments).") == 0){
				printf("Invalid arguments\n");
			}
			else if(strstr(buffer,"200 Command") !=0){
				printf("%.*s",l,buffer);
			//	printf("DONE\n");
			}
			break;
		} else{
			//if(strncmp(buffer,"
			//break;
			printf("%s",buffer);
		}
	}while(1);
	char temp[MAX];
	memset(temp,0,MAX);
	recv(cmdSock,temp,MAX,0);
	//printf("temp is [%s]\n",temp);
	if(strcmp(temp,"200 Command OK.") == 0){
		//printf(" GOT OK, buffer is: [%s]\n",temp);
	}
	else if(strcmp(temp,"501 Syntax error (invalid arguments).") == 0){
		//printf("No such file or directory\n");
		printf("Invalid command: wrong arguments\n");
	}
	//close data socket
	close(dataSock); 
	
}

int dataConn(){
	int dataSock;
	if((dataSock= socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("can't make dataConn socket");
		exit(1);
	}
	
	struct sockaddr_in server_data;
	struct sockaddr_in client_data;
	
	//server.sin_addr.s_addr = htons(atoi(ip));//sets addr to any interface
	int option = 1;
	setsockopt(dataSock,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &option,sizeof(option));
	server_data.sin_family = AF_INET;
    server_data.sin_addr.s_addr = INADDR_ANY;
    server_data.sin_port = htons(dataPort);
	
	if (bind(dataSock, (struct sockaddr *) &server_data, sizeof(server_data)) < 0){
		perror("Can't bind to data conn\n");
	}
	listen(dataSock, 1);
	
	socklen_t len = sizeof(client_data);
	
	int finalDataSock;
	if((finalDataSock= accept(dataSock, (struct sockaddr *) &client_data, &len))<0){
		perror("Can't accept new data connection");
	}
	close(dataSock);
	return finalDataSock;
	
}

int cmdConn(int port){
	//crate a socket
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	//check for errors
	if(socketFD <0){
		printf("Error: Can't make a socket\n");
		exit(1);
	}
	
	//set up attributes
	server.sin_family = AF_INET; //set connection to TCP
	server.sin_port = htons(port); //assigns port number
	
	struct hostent *host;
	struct sockaddr_in data_addr;
	host = gethostbyname(ip);
					
	// check for errors
    if (host == NULL) {
        printf("Host error\n");
        exit(0);
    }
	
	inet_pton(AF_INET, ip, &data_addr.sin_addr);
	
	struct in_addr **address;
	address = (struct in_addr **) host->h_addr_list;
	char dest[100];
	strcpy(dest,inet_ntoa(*address[0]));
	inet_pton(AF_INET,dest,&(data_addr.sin_addr));
	
	//server.sin_addr.s_addr = htons(atoi(ip));//sets addr to any interface
	int option = 1;
	setsockopt(socketFD,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &option,sizeof(option));
	
	if(connect(socketFD, (struct sockaddr *)&server, sizeof(server))<0){
		perror("Failed to connect");
		exit(1);
	}
	return socketFD;
	
}

//CheckArgs() checks for valid arguments
int checkArgs(int *args, char *argv[]){
	//Make sure we have right number of args
	if(*args !=3){ //supposed to be 4
		printf("Usage: ./ftpclient <server-ip>	<server-listen-port>\n");
		exit(1);
	}
	strncpy(ip,argv[1],IPMAX);
	int port = atoi(argv[2]);
	if(port <=0){
		printf("invalid port\n");	
		exit(1);
	}
	return port;
}

//when CTRL C is clicked, ask
//user if they want to quit
void  CTRLc(int sig)
{
     //char  c;

     signal(sig, SIG_IGN);
     //printf("\nDo you really want to quit the server? [y/n]: ");
    // c = getchar();
    // if (c == 'y' || c == 'Y'){
        send(socketFD,"ABOR",4,0); 
		  	//free result to avoid mem leaks
		//free(result);
		close(socketFD);//close cmdSock
		printf("\n");
	 exit(0);
   // signal(SIGINT, CTRLc);
}