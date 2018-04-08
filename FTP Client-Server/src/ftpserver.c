//             Ramzey Ghanaim
//                ftpserver.c
//                  Final Lab
//     
//
//  This program implements a porxy server
//  that filters websites through a web
//  browser
//  TCP Transpotr layer protocol.
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
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>

typedef struct{
	int cmdSock;
	char buffer[MAX];
}tdata;


int checkArgs(int *args, char *argv[]);
void cmdConn(int port);
int dataConn(int dataPort);
void* executeCmd(void *tdata);
void sendAck(char *msg, int cmdSock);
int socketFD, dataPort = 0;
char ip[IPMAX];
//socket info about the client connecting to this server
struct sockaddr_in client;

//socket info about this server
struct sockaddr_in server;
int abortFlag = 0;
int exitFlag = 0;
int done;
int dataSock=0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int main(int argc, char* argv[]){
	int port = checkArgs(&argc,argv);
	cmdConn(port);
	int addrSize = sizeof(struct sockaddr_in);
	int cmdSock;
	printf("Waiting for clients...\n");
	//char buffer[MAX];
	pthread_t t;
//	int pid;
	//int oldPid;
	
	tdata threadData;
	
	int size; //size of buffer
	//threadData.cmdSock = malloc(sizeof(int));
	while((cmdSock = accept(socketFD,
							 (struct sockaddr *)&client,
							 (socklen_t*)&addrSize)) >0){
		
		threadData.cmdSock = cmdSock;
		//memset(&buffer,0,MAX);
		//int flag = 1;
		
		while(!exitFlag){
			if( (size = recv(cmdSock,threadData.buffer,sizeof(threadData.buffer),0) )< 0){
				perror("Can't recv");
			}
			else{
				//printf("  sdkfjaksldfjdkasj   GOT: [%s]\n",threadData.buffer);
				if(size > 0){
					if(strncmp(threadData.buffer,"ABOR",4) == 0){
						printf("GOt Abort\n");
						//close(dataSock);
						//close(cmdSock);
						pthread_cancel(t);
						exitFlag = 1;
						//goto begin;
					}
					else if(strlen(threadData.buffer)>1){
						pthread_create(&t,NULL,executeCmd,&threadData);
					}
				}
			}
			
		//printf("EXIT FLag = 0\n");
		
		}
		exitFlag = 0;
			printf("Waiting for clients...\n");
		//} else{
		//	oldPid = pid;
		//}
		
	}
	printf("Accept is: %i\n",cmdSock);
	return 0;
}
void* executeCmd(void *tData){
	tdata *threadData = (tdata*) tData;
	int cmdSock = (*threadData).cmdSock;
	char buffer[MAX];
	//int size; //size of buffer
	char *token;
	//char *space;
	int counter = 0;
	int n5 = 0,n6=0;
	strncpy(buffer,(*threadData).buffer,MAX);
	//while(!exitFlag){
		
		//printf("BUFFER IS : [%s]\n",buffer);
		
		if(strncmp(buffer,"PORT", 4) == 0){
			printf("GOT [%s]\n",buffer);
			//get past cmd
			token = strtok(buffer," ");
			//printf("token is: [%s]",space);
			//memset(space,0,sizeof(space));
			//grab data based on PORT format
			token = strtok(NULL, ",");
			while( token != NULL ){
				if(counter == 0){
					strncpy(ip,token,IPMAX);
					strncat(ip,".",1);
					printf("token is: [%s]\n",token);
				}
				
				//copy ip address
				else if(counter <4){
					strncat(ip,token,IPMAX);
					if(counter <3){
						strncat(ip,".",1);
					}
				}
				//get port
				else if(counter == 4){
					n5=atoi(token);
				}
				else if(counter == 5){
					n6=atoi(token);
					dataPort = (n5*256)+n6;
				}
				++counter;
				token = strtok(NULL, ",");
			}
			//printf("IP IS: [%s] ",ip);
			//printf("port is: [%i]\n",dataPort);
			//respond to client 200 OK
			sendAck("200 Command OK.",cmdSock);
		
		}// end PORT command
		
		//quit program
		else if(strncmp(buffer,"QUIT", 4) == 0){
			sendAck("221 Goodbye.",cmdSock);
			//abortFlag = 1;
			done = 1;
			exitFlag = 1;
		}
		
		//abort previous FTP command and any data transfer
		else if(strncmp(buffer,"ABOR",4) == 0){
			//exitFlag = 1;
			//done = 1;
			//abortFlag = 1;
			//return exitFlag;
			return NULL;
		}
		
		//list files and dirrectories
		else if(strncmp(buffer,"LIST",4) == 0){
			//sendAck("200 Command OK.",cmdSock);
			printf("GOT [%s]\n",buffer);
			//abortFlag = 1;
			int dataSock = dataConn(dataPort);
			//get past cmd
			// if(done){
					// //printf("Retruning\n");
					// close(dataSock);
					// //close(cmdSock);
					// return NULL;
				// }
			printf("Data socket in func is: %i\n",dataSock);
			char cmd[MAX];
			if(strlen(buffer) ==4){
				strcpy(cmd,"ls .");
			}else{
				strcpy(cmd,"ls ");
				token = strtok(buffer," ");
				token = strtok(NULL,"\n");
				strcat(cmd,token);
			}
			
			printf("About to execute :[%s]\n",cmd);
			FILE *ptr;
			if((ptr= popen(cmd,"r")) == NULL){
				perror("Can't popen on ls cmd");
				exit(1);
			}
			int donet = 0;
			int bad = 0;
			int first = 1;
			//printf(" Pointer is: [%s]",ptr);
			do{
				memset(buffer,0,MAX);
				
				if( fgets(buffer, MAX, ptr) == NULL){
					donet = 1;
					sendAck("200 Command OK.",dataSock);
					if(first){
						bad = 1;
					}
					
				}else{
					first = 0;
					//printf("About to send: [%s]\n",buffer);
					if(strstr(buffer,"ls: cannot access ") != 0){
						printf("in the if statement\n");
						sendAck("200 Command OK.",dataSock);
						
						donet = 1;
						bad = 1;
					}else{
						printf("len ofpointer is : %i\n",(int) strlen(buffer));
						printf(" Pointer is: [%s]",buffer);
						sendAck(buffer,dataSock);
					}
				}
				
				
			}while( !donet);
			if(bad==0){
				printf("HERE\n");
				sendAck("200 Command OK.",cmdSock);
			}else{
				printf("HERE 2\n");
				sendAck("501 Syntax error (invalid arguments).",cmdSock);
			}
			close(dataSock);
			//printf("     DONE SENDING\n");
			return NULL;
		}
		
		//retrieve a file (send to client)
		else if(strncmp(buffer,"RETR",4) == 0){
			sendAck("200 Command OK.",cmdSock);
			printf("GOT [%s]\n",buffer);
			
			int dataSock = dataConn(dataPort);
			char *fileName = strtok(buffer," ");
			fileName = strtok(NULL,"\n");
			//printf("FILE NAME IS:[%s]\n",fileName);
			FILE *fileptr;
			if((fileptr = fopen(fileName,"r")) == NULL){
				printf("FILE NOT FOUND\n");
				sendAck("501 Syntax error (invalid arguments).",dataSock);
			}
			else{
				//get to end of file 
				fseek(fileptr,0,SEEK_END);
				
				//pointer location is now the 
				//size of the file
				int fileLen = ftell(fileptr);
				
				//put pointer back to beginning
				rewind(fileptr);

				//get message ready to send
				sprintf(buffer,"%d",fileLen);
				
				//printf("file size: %i bytes\n", fileLen);
				//send file size
				sendAck(buffer,dataSock);
				// printf("Waiting for ack\n");
				// recv(dataSock,buffer,MAX,0);
				// if(strcmp(buffer,"ACK") == 0){
					// printf("got ack\n");
				// }
				//printf("GOT ACK\n");
				
				
				// struct stat file_stat;
				// if(fstat(fileD, &file_stat) < 0){
					// printf("CAN'T GET STATS\n");
				// }
				int sent;
				//off_t offset = 0;
				int remain = fileLen;
				abortFlag = 1;
				sleep(1);
				//printf("File size is: %i",(int) file_stat.st_size);
				while(1){//1
					memset(buffer,0,MAX);
					if((sent = fread(buffer, 1, MAX, fileptr))==0){
						break;
					}
					//printf("sent data is: %i\n",sent);
					//printf("About to send[%s]\n",buffer);
					remain -=sent;
					send(dataSock,buffer,MAX,0);
					if(remain ==0){
						break;
					}
				}
					
					//sendfile(dataSock, fileD, 0, remain);
				//	printf("DOne sending\n");
				fclose(fileptr);
				//close(fileD);
			}
			sleep(1);
			close(dataSock);
		}
		
		//store a file (get from client)
		else if(strncmp(buffer,"STOR",4) == 0){
			sendAck("200 Command OK.",cmdSock);
			printf("GOT [%s]\n",buffer);
			
			int dataSock = dataConn(dataPort);
			char *fileName = strtok(buffer," ");
			fileName = strtok(NULL,"\n");
			printf("FILE NAME IS:[%s]\n",fileName);
			FILE *fileptr;
			if((fileptr = fopen(fileName,"w")) == NULL){
				printf("FILE NOT FOUND\n");
				sendAck("452 Error writing file.",dataSock);
			}
			else{
				char fileLen[MAX];
				if(recv(dataSock,fileLen,MAX,0) <0){
					perror("Can't get file len");
				}
				else{
					int fileSize = atoi(fileLen);
					printf("FILE SIZE IS: %i\n",fileSize);					
					int got_data;
					int remain_data = fileSize;
					int total = 0;
					//sleep(1);
					while(remain_data > 0){ //remain_data > 0
						//printf("HERE\n");
						memset(buffer,0,MAX);
						//printf("ABOUT TO GET DATA\n");
						if((got_data = recv(dataSock,buffer,MAX,0) )<=0){
							printf("GOT NOTHING\n");
							break;
						}
						//printf("%s",buffer);
						if(total == fileSize){
							printf("HERE\n");
							break;
						} 
						fwrite(buffer,1,strnlen(buffer,MAX),fileptr);
						remain_data -=got_data;
						total +=got_data;
					}
				//	printf("last of the data is \n%s\n",buffer);
					
					// memset(buffer,0,MAX);
					// recv(cmdSock,buffer,MAX,0);
					//printf("BUFFER IS: [%s]\n",buffer);
					
					printf("GOT %i\n",total);
					fclose(fileptr);
				}
			}
			close(dataSock);
		}
		
		//no valid commands
		else{
			
			int r = strlen(buffer);
		//	printf("Len is: %i\n",r);
			if(r >0){
				printf("ABOUT TO SEND ERROR\n");
				sendAck("500 Syntax error.",cmdSock);
			}
		}
	//}
	done = 1;
	return NULL;
}


void sendAck(char *msg, int cmdSock){
	//printf("ABOUT TO SEND: [%s]\n",msg);
	if( send(cmdSock,msg,strlen(msg),0)  < 0){
		perror("Failed to send good port");
		done = 1;
		abortFlag = 1;
		return;
		//exit(1);
	}
	//memset(msg,0,MAX);
}

int dataConn(int dataPort){
	struct hostent *host;
	struct sockaddr_in data_addr;
	int dataSock;
	if( (dataSock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Can't open data socket \n");
		//pthread_mutex_unlock(&mutex);
        exit(1);
	}

	pthread_mutex_lock(&m);
	if((host = gethostbyname(ip))== NULL){
        printf("Host error\n");
		pthread_mutex_unlock(&m);
        exit(0);
    }
	pthread_mutex_unlock(&m);
	int temp = dataPort;
	data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(temp);
    inet_pton(AF_INET, ip, &data_addr.sin_addr);
	
	struct in_addr **address;
	address = (struct in_addr **) host->h_addr_list;
	char dest[100];
	strcpy(dest,inet_ntoa(*address[0]));
	inet_pton(AF_INET,dest,&(data_addr.sin_addr));
	sleep(1);
	 if(connect(dataSock, (struct sockaddr*)&data_addr, sizeof(struct sockaddr)) < 0){
        perror("Can't connect to data channel");
		exitFlag = 1;
		done = 1;
       return 0;
    }
    printf("Connected to Client on DATA CHANNEL \n ");
    return dataSock; // returning the file descriptor obtained for data transfer
	
	
}



void cmdConn(int port){
	//crate a socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	//check for errors
	if(socketFD <0){
		printf("Error: Can't make a socket\n");
		exit(1);
	}
	
	//set up attributes
	server.sin_family = AF_INET; //set connection to TCP
	server.sin_port = htons(port); //assigns port number
	server.sin_addr.s_addr = htons(INADDR_ANY);//sets addr to any interface
	int option = 1;
	setsockopt(socketFD,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &option,sizeof(option));
	
	
	
	//bind server information to socket
	if(bind(socketFD, (struct sockaddr *) &server, sizeof(server)) < 0){
		printf("can't bind\n");
		exit(1);
	}
	//start listening for clients, on the socket we made
	//and listen for up to 501 clients at a time
	listen(socketFD,501);
	
}



//CheckArgs() checks for valid arguments
int checkArgs(int *args, char *argv[]){
	//Make sure we have right number of args
	if(*args !=2){ //supposed to be 4
		printf("Usage: ./ftpserver <port> \n");
		exit(1);
	}
	int x;
	for(x = 0; x<strlen(argv[1]);++x){
		if(argv[1][x] <'0' && argv[1][x] > '9'){
			printf("Port must be a number\n");
			exit(1);
		}
	}
	int port = atoi(argv[1]);
	if(port <=0){
		printf("Invalid Port\n");	
		exit(1);
	}
	return port;
}