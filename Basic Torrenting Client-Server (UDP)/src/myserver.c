//         Ramzey Ghanaim
//            server.c
//             Lab 3
//
//  This server is used as a seeder to seed
//  a specified ammount of data from a text file
//  to client
//  
//


#define MAX 1000
#define MSG 10
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h> //CTRL C handeling
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

//FILE *fileptr;
typedef struct{
	int maxByte;
	int minByte;
	int outOf;
	int index;
	int serverNum;
	char buffer[MAX];
	char msgType[10];
	int serverPort;
	struct sockaddr_in client;
	struct sockaddr_in server;
	int socketFD;
	int *error;
}message;


int fileLen; //size of the file
int socketFD;
FILE *fileptr;
void sendFile(message getData, message sendData);
struct timeval tv;
int main(int argc, char* argv[]){
	
	//if one argument assume it is a port
	if(argc <2){
		printf("Usage: <Port>\n");
		exit(1);
	}
	//If user wants to check robustness, 
	//turn on error flag
	int exitFlag = 0;
	if(argc == 3 && strcmp(argv[2], "e") == 0){
		printf("Exit flag is on\n");
		exitFlag = 1;
	}
	
	//socket info about the client connecting to this server
	struct sockaddr_in client;

	//socket info about this server
	struct sockaddr_in server;
	
	//crate a socket
	int socketFD = socket(AF_INET, SOCK_DGRAM, 0);
	
	
	//set up attributes
	server.sin_family = AF_INET; //set connection to TCP
	server.sin_port = htons(atoi(argv[1])); //assigns port number
	server.sin_addr.s_addr = INADDR_ANY; //sets addr to any interface
	
	
	//bind server information to socket
	if(bind(socketFD, (struct sockaddr *) &server, sizeof(server)) <0){
		perror("can't bind\n");
		close(socketFD);
		exit(1);
	}
	
	
	
	//struct message *getData = malloc(sizeof(struct message));
	message getData;
	message sendData;
	socklen_t csize= sizeof(client);
	memset(&getData.buffer,0,MAX);
	begin:
	printf("Waiting for client...\n");
	if(recvfrom(socketFD,&getData,sizeof(getData),0,
				(struct sockaddr *) &client, &csize) < 0){
			perror("Timeout\n");
	}
	else{
		//exit(1);
		printf("Got succesfully\n");
		printf("Buffer is[%s]\n",getData.buffer);
	}	
	int done = 0;
	if(getData.serverPort <0 ){
		int pid = fork();
		
		if(pid == 0){ //I am child
		
		
		printf("IN CHILD\n");
		close(socketFD);
		
		sendData.socketFD = socket(AF_INET, SOCK_DGRAM, 0);
		sendData.client = client;
		sendData.server = server;
		//memset(&(sendData.server), 0, sizeof(sendData.server));		
		//getsockname(getData.socketFD,(struct sockaddr *) &(getData.server), sizeof(getData.server));
		
		//set up attributes
		sendData.server.sin_family = AF_INET; //set connection to TCP
		sendData.server.sin_port = 0; //assigns port number
		sendData.server.sin_addr.s_addr = INADDR_ANY; //sets addr to any interface
		int sendsize = sizeof(sendData.server);
		socklen_t len = sizeof(server); 
		if(bind(sendData.socketFD, (struct sockaddr *) &(sendData.server), sendsize) <0){
			perror("can't bind\n");
		}	
		//In a forked proccess, assign a new port to use for the server
		getsockname(sendData.socketFD,(struct sockaddr *) &(sendData.server), &len);
		//tell the client which port to use
		sendData.serverPort = ntohs(sendData.server.sin_port);
		
		
		//set Timeout times
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		setsockopt(sendData.socketFD, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
		
			printf("OUT OF is[%i] and index: %i\n",getData.outOf,getData.index);
			printf("MSG is: [%s]  buffer is [%s]\n",getData.msgType,getData.buffer);
			printf("new port is %i\n",sendData.serverPort);
			while(!done){
				//if server wants to get file size....
				if(strncmp(getData.msgType,"gFileSize", MSG) ==0){
					printf("file name is [%s]\n",getData.buffer);
					
					fileptr=fopen(getData.buffer,"r");
					if(fileptr == NULL) {
						//prompt client
						printf("FILE NOT FOUND\n");
						strncpy(sendData.msgType,"FNF",MSG);
						sendData.outOf = 1;
						sendData.index = 1;
						if(sendto(sendData.socketFD, (message *) &sendData, (1024+sizeof(sendData)),
								0,(struct sockaddr *) &(client),sizeof(client)) < 0){
									perror("FAILED to send: FNF\n");
								}
					}
					else{ //file is valid, send file size
						//get to end of file 
						
								fseek(fileptr,0,SEEK_END);
								
								//pointer location is now the 
								//size of the file
								fileLen = ftell(fileptr);
						
								//put pointer back to beginning
								rewind(fileptr);
								
								strncpy(sendData.msgType,"FileSize", MSG);
								sprintf(sendData.buffer,"%d",fileLen);
								sendData.outOf = 1;
								sendData.index = 1;
								
								//printf("4.new port is %i\n",sendData.serverPort);
								if(sendto(sendData.socketFD, (void *)&sendData, (sizeof(sendData)),
											0,(struct sockaddr *) &client,sizeof(client)) < 0 ){
									perror("FAILDED TO SEND\n");
								}
								else{
									printf("SENT SUCCESSFULLY\n");
								}
						
									
								
					}// end else
				}// end gFileSize
			
				//open file the client wants and send an ACK
				if(strncmp(getData.msgType,"FileSize", MSG) ==0){
					fileptr = fopen(getData.buffer,"r");
					if(fileptr == NULL){
						printf("unsuccessful\n");
						strncpy(sendData.msgType,"FNF",MSG);
						sendData.outOf = 1;
						sendData.index = 1;
						if(sendto(sendData.socketFD, (message *)&sendData, (sizeof(sendData)),
								0,(struct sockaddr *) &(sendData.client),sizeof(sendData.client)) < 0){
									perror("FAILED to send: FNF\n");
						}
					} //end if fileptr is null
					else{ //if we got the file and are just acking
						strncpy(sendData.msgType,"ACK",MSG);
						sendData.outOf = 1;
						sendData.index = 1;
						if(sendto(sendData.socketFD, (void *)&sendData, (sizeof(sendData)),
											0,(struct sockaddr *) &client,sizeof(client)) < 0 ){
									perror("FAILDED TO SEND\n");
						}
						else{
							printf("SENT SUCCESSFULL ACK\n");
						}
						
						
					}
					
				}
				
				//if the client wants the server to "give bytes", we send it
				if(strncmp(getData.msgType,"givBytes",MSG) == 0){ //send data
					printf(" SOCKET WE GOt IT ON IS: %i\n",getData.socketFD);
					if(exitFlag){
						printf("exit flag is on. Good Bye\n");
						done = 1;
						exit(0);
					}
						strncpy(sendData.msgType,"ACK",MSG);
						sendData.outOf = 1;
						sendData.index = 1;
						if(sendto(sendData.socketFD, (void *)&sendData, (sizeof(sendData)),
											0,(struct sockaddr *) &client,sizeof(client)) < 0 ){
									perror("FAILDED TO SEND\n");
						}
						else{
							printf("SENT SUCCESSFULL ACK\n");
						}
					sendFile(getData, sendData);
				}
				
				//get ACK
				printf("Waiting for ack in child...\n");
				int flag = 0;
				tryAgain:
				if(recvfrom(socketFD,&getData,sizeof(getData),0,
							(struct sockaddr *) &client, &csize) < 0){
						perror("Timeout\n");
						++flag;
						if(flag<6){ //up to 6 times for 30sec wait
							goto tryAgain;
						}
						else{
							printf("client must have gone missing. Exiting child\n");
							exit(1);
						}
				}
				else{//we got ack
					
					printf("Got succesfully\n");
					printf("Buffer is[%s]\n",getData.msgType);
					//printf("min bytes: %i   max bytes: %i\n",getData.minByte,getData.maxByte);

				}	
				//if client says we are done, send ack and exit child
				if(strncmp(getData.msgType,"Done",MSG) == 0){
					done = 1;
					//close(sendData.socketFD);
					int d = 0;
					strncpy(sendData.msgType,"ACK",MSG);
					sends:
						if(sendto(sendData.socketFD, (message *) &sendData, (1024+sizeof(sendData)),
								0,(struct sockaddr *) &(client),sizeof(client)) < 0){
									perror("FAILED to send: FNF\n");
									++d;
									if(d<5) //send up to 3 times.
										goto sends;
								}
					exit(0);
				}
				
			
			}//end while(!done)
		}// end fork
	//int status;
	//wait(&status);
	
	goto begin;
	

	}// end if port == null
	return 0;
}//end main

//sendFile() deals with the parsing of the file and
// sending of data
void sendFile(message getData, message sendData){
	printf("min bytes: %i   max bytes: %i\n",getData.minByte,getData.maxByte);
	int totalChunk = getData.maxByte - getData.minByte;
	//printf("===total chunk is: %i\n", totalChunk);
	message sData;
	sData.socketFD = sendData.socketFD;
	fseek(fileptr,getData.minByte,SEEK_SET);
//if the total size of the chunk is bigger than one
// packet size, split it up:
	if(totalChunk > MAX){
	//	printf("          TOTAL > CHUNCK\n");
		int outOf = totalChunk/MAX;
		int r = totalChunk % MAX;
		if(r > 0){
			outOf++;
			
		}
		printf("out of is: %i   remainder is : %i",outOf, r);
		printf("total chunck size is: %i\n",r+ (outOf *MAX));
		int i;
		int tempMin = getData.minByte;
		for(i = 0;i<outOf;++i){
			sendData.outOf = outOf;
			sendData.index = (i+1);
			
			//read data from the file pointer location
			//up until the max size of the buffer -1
			
			if(i == outOf-1 && r >0 ){
				// sData.buffer[r+1] = 0;
				//printf("server num is %i\n", sData.serverNum);
				if(getData.serverNum == 69){
					++r;
					if(getData.maxByte < fileLen){
						++r;
					}
					getData.serverNum = 0;
					//printf("HERE\n");
				}
				fread(sData.buffer, 1, r, fileptr);
			 }
			 else{
				 fread(sData.buffer, 1, MAX, fileptr);
			 }
			//fseek(fileptr,getData.maxByte-1,SEEK_SET);
			
				//printf("pointer location AFTER: %ld bytes\n", ftell(fileptr));
				//printf("BEGIN: [%s]\n", sData.buffer);
			
			if(sendto(sData.socketFD, (void *)&sData, (sizeof(sData)),
			 0,(struct sockaddr *) &(sendData.client),sizeof(sendData.client)) < 0 ){
				perror("Failed to send data succ ***\n");
				memset(&(sData.buffer),0,MAX);
			}
			else{
				//printf("len is: %zu\n",strnlen(sData.buffer,MAX));
				memset(&(sData.buffer),0,MAX);
				
			}
			tempMin +=MAX;
			//printf("Next temp Min is: %i\n",tempMin);
		}
		
		return;
	}
	//Do normal sending if we don't need to send multiple 
	//packets
	
	
	//go to the minByte location
	if(getData.maxByte == fileLen){
		totalChunk --;
	}
	
	//one packet out of one
	sData.outOf = 1;
	sData.index = 1;
	//read data from the file pointer location
	//up until the max size of the buffer -1
	fread(sData.buffer, 1, fileLen, fileptr);
	printf("pointer location AFTER: %ld bytes\n", ftell(fileptr));
	sData.buffer[fileLen] = 0;
	printf("ABOUT TO SEND: [%s]\n", sData.buffer);

	if(sendto(sData.socketFD, (void *)&sData, (sizeof(sData)),
			 0,(struct sockaddr *) &(sendData.client),sizeof(sendData.client)) < 0 ){
		perror("Failed to send data succ ***\n");
	}
	else{
		printf("sent data success **\n");
	}

	
}