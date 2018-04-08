//             Ramzey Ghanaim
//                myclient.c
//                  Lab 3
//     
//
//  This program implements a torrenting like
//  download format from multiple servers using
//  UDP Transpotr layer protocol.
//
#define MAX 1000
#define LIST 100
#define MSG 20
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <pthread.h>

//message struct keeps track of 
//all attributes a packet needs.
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

//------------Prototypes-----------------------------------------
int checkArgs(int *args, char *argv[]);

int findServers(char IPlist[LIST][LIST], char portList[LIST][LIST], char *fileName);
int makeConnections(char IPlist[LIST][LIST], char portList[LIST][LIST],
					int *socketFD, int *serverList, int min, int *index,
					struct sockaddr_in *serverAddr, int serverCount,
					char *fileName);
 void setUpMsg(message *sendData, message *recvData, int succesServers, int divSize,
			  int remainder, int *serverList, int *socketFD);
void getFile(int *socketFD, int *serverList, int divSize,int remainder, int succesServers,
             struct sockaddr_in *serverAddr, message *sendData, message *recvData, char *name);
//--------------------------------------------------------------------------------------

//public variables: file size and timeout
int fileSize;
struct timeval tv;


int main(int argc, char* argv[]){					
	//create a list of ports and associated IPs
	char IPlist[LIST][LIST];
	char portList[LIST][LIST];
	int i;
	

	//fill lists
	//keep track of number of servers available
	int serverCount = findServers(IPlist, portList, argv[1]);
	for(i=0;i<serverCount;++i){
		printf("IP: [%s]",IPlist[i]);
		printf("   PORT [%s]\n",portList[i]);
	}
	printf("we found %i servers \n", serverCount);
	//get number of requested servers
	int userReq = atoi(argv[2]);
	
	//calculate the min between server in the text file
	// and servers requested by the user
	int min = (serverCount) <= (userReq) ? (serverCount) : (userReq);
	printf("min is: %i\n",min);
	if(min == 2){
		min++;
	}
	
	//initalizations
	int socketFD[min];
	int serverList[serverCount];
	struct sockaddr_in serverAddr[min];
	int index = 0;
	memset(socketFD, -5, serverCount);

	//set Timeout times
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	char *name = argv[3];
	//make connections with found servers
	int succesServers = makeConnections(IPlist, portList,socketFD, 
										serverList, min, &index,
										serverAddr, serverCount,
										name);
	
	//sent new timeout
	tv.tv_sec = 3;
	printf("We got %i Servers\n",succesServers);

	int divSize = fileSize/succesServers;
	int remainder = divSize % succesServers;
	int totalSize = (divSize * succesServers)+remainder;
	//int maxBuffer = divSize+remainder;
	printf("total size is: %i\n",totalSize);
	
	message sendData[succesServers];
	message recvData[succesServers];
	
	setUpMsg(sendData,recvData,succesServers,divSize,remainder,
			  serverList, socketFD);

	getFile(socketFD, serverList,divSize,remainder, succesServers,
			serverAddr, sendData,recvData, name);
	
	return 0;
}

//divides up the message based off the number of servers 
// that are found
 void setUpMsg(message *sendData, message *recvData, int succesServers, int divSize,
			  int remainder, int *serverList, int *socketFD){

//   The goal is to iterate through for each server and assign it 
//   a specific min and max byte, along with an error flag, and index
//   and a buffer to store a fragment of the file in
	
	//size of data we want to send each server
	int dataSize=0, i, nextMin = 0; 
	
	printf("success servers is: %i\n",succesServers);
	//Don't iterate through last file, we simply
	//put the remainder + dataSize here
	for(i=0;i<succesServers-1;++i){
			if(i==0){
				sendData[i].minByte = 0;
				sendData[i].maxByte = divSize;
				printf("thread %i: Min = %i     thread Max= %i\n", i, sendData[i].minByte, sendData[i].maxByte);
				
			}
			else{
				sendData[i].minByte = nextMin;
				sendData[i].maxByte = nextMin+divSize-1;
				printf("thread %i: Min = %i   thread Max = %i\n", i, sendData[i].minByte, sendData[i].maxByte);

			}
				//DEBUG: printf("index = %i,   socket fd is: %i\n", successServerIndex[i],socketFD[successServerIndex[i]]);
				sendData[i].socketFD = socketFD[serverList[i]];
				nextMin = sendData[i].maxByte; //removed +1
				sendData[i].error = malloc(sizeof(int));
				sendData[i].error = 0;
				sendData[i].index = i;
	}
	
	//get size + remainder data from last server
	sendData[i].minByte = nextMin;
	sendData[i].maxByte = nextMin+divSize+remainder;
	sendData[i].socketFD = socketFD[serverList[i]]; //serverList[i]
	sendData[i].error = malloc(sizeof(int));
	sendData[i].error = 0;
	printf("process %i: Min = %i     process Max= %i\n", i, sendData[i].minByte, sendData[i].maxByte);

	if(succesServers == 1){
		dataSize = divSize+1;
	}else
		dataSize = (sendData[0].maxByte-sendData[0].minByte)+1;
	printf("Data size is: %i\n",dataSize);
	
}

//getFile()  iterates though the available servers,
// downloads the file, and saves it into a new file
void getFile(int *socketFD, int *serverList, int divSize,int remainder, int succesServers,
             struct sockaddr_in *serverAddr, message *sendData, message *recvData, char *name){
	
	FILE *finalFile;

	finalFile=fopen(name,"w");
	
	int totalChunk;
	
	int i, flag= 0, index = 0, goodIndex = 0;
	int errors =0;
	//int bad[succesServers];
	int good[succesServers];
	int x;
	int outOf;
	
	int r;
		
	
	for(i=0;i<succesServers;++i){
		strcpy(sendData[i].msgType,"givBytes");
		printf("New send data type: [%s]\n",sendData->msgType);
		totalChunk= sendData[i].maxByte - sendData[i].minByte;
		outOf = totalChunk/MAX;
		r = totalChunk % MAX;
		if(r > 0 ){
			outOf++;
	
			if(i == succesServers-1 && fileSize%MAX !=fileSize){
				sendData[i].serverNum = 69;
			}
		}
		
	//DEBGUG:printf("min is: %i   max is: %i  out of is: %i remainder is %i total chunks is: %i\n",sendData[i].minByte, sendData[i].maxByte, outOf, r,totalChunk);
		do{
			
			//DEBUG:printf("ABOUT TO SEND: minByte: %i    maxByte:%i\n",sendData[i].minByte,sendData[i].maxByte);
			sendData->serverNum = i;
			socklen_t temp = sizeof(recvData[i]);
			//send the chunck we want to the server we want
			b:
			if(sendto(socketFD[serverList[index]], (void *)&sendData[i], (sizeof(sendData[i])),
				0,(struct sockaddr *) &serverAddr[serverList[index]],sizeof(serverAddr[serverList[index]])) < 0 ){
					perror("FAILDED TO SEND\n");
			}
			if(recvfrom(socketFD[serverList[index]],&recvData[i],sizeof(recvData[i]),
						0, (struct sockaddr *) &serverAddr[serverList[index]], &temp) < 0){
					perror("no ACk"); flag++;
					if(flag >=5){
						flag = 0;
					errors++;
						printf("Getting new Server\n");
						//bad[badIndex] = i;
						++index;
						if(errors == succesServers){
							printf("No good servers. Exiting now\n");
							exit(1);
						}
						if(index == succesServers && goodIndex>0){
							index = good[goodIndex-1];// use the prev good server
							
						}
						if(index == succesServers){
							index = 0;
						}
					}
					goto b;
				
			}
			 memset(recvData->msgType,0,MSG);
			

			//recieve the file based off of the number of 
			//packets being sent(outOf)
			for(x=0;x<outOf;++x){
				
				if(recvfrom(socketFD[serverList[index]],&recvData[i],sizeof(recvData[i]),
						0, (struct sockaddr *) &serverAddr[serverList[index]], &temp) < 0){
					perror("Timeout");
					printf("ABOUT TO SEND: minByte: %i    maxByte:%i\n",sendData[i].minByte,sendData[i].maxByte);
					if(sendto(socketFD[serverList[index]], (void *)&sendData[i], (sizeof(sendData[i])),
								0,(struct sockaddr *) &serverAddr[serverList[index]],sizeof(serverAddr[serverList[index]])) < 0 ){
						perror("FAILDED TO SEND\n");
					}
					++flag;
					if(flag >=5){
						
						errors++;
						printf("Getting new Server\n");
					//	bad[badIndex] = i;
						++index;
						if(errors == succesServers){
							printf("No good servers. Exiting now\n");
							exit(1);
						}
						if(index == succesServers && goodIndex>0){
							index = good[goodIndex-1];// use the prev good server
							
						}
						if(index == succesServers){
							index = 0;
							printf("HERE\n");
						}
						break;
						// recvData[index].minByte = recvData[i].minByte;
						// recvData[index].maxByte = recvData[i].minByte;
						
					}
				}
				else{ //got data succesfully
					//printf("%s",recvData[i].buffer);
					good[goodIndex] = index;
					//printf("GOOD INDEX is: %i\n",good[goodIndex]);
					
					//write data to the file
					fwrite(recvData[i].buffer,1,strnlen(recvData[i].buffer, MAX),finalFile);
					memset(&(recvData[i].buffer),0,MAX);
					if(x == outOf-1){
						++goodIndex;
						++index;
						//if you went through all servers and there are no errors
						if(index == succesServers && errors == 0){
								printf("Done\n");
								//printf("len of buffer: %zd\n",strnlen(recvData[i].buffer, MAX));
						}
						if(errors > 0){
							--index;
							--errors;
						
						}
						if(errors == 0){
							flag = 0;
						}
					}
					
					
					
				}//end got data successfully
			}//end outOf
		}//end do
		 while(flag > 0);
	}// end for
	index = 0;
	int d;
	socklen_t e;
	
	
	//the following code tells each server 
	//to shut down by sending the "done" 
	// message
	printf("telling servers we are done...\n");
	for(i = 0;i<succesServers;++i){
		strcpy(sendData[i].msgType,"Done");
	
		d=0;
		sendA:
		if(sendto(socketFD[serverList[good[index]]], (void *)&sendData[i], (sizeof(sendData[i])),
				0,(struct sockaddr *) &serverAddr[serverList[index]],sizeof(serverAddr[serverList[good[index]]])) < 0 ){
					perror("FAILDED TO SEND");
					if(++d < 5){ //send up to 5 times
						 goto sendA;
					}
		}
		
		e = sizeof(serverAddr[serverList[index]]);
		if(recvfrom(socketFD[serverList[index]],&recvData[i],sizeof(recvData[i]),
						0, (struct sockaddr *) &serverAddr[serverList[index]], &e) < 0){
			if(++d < 5){ //send up to 5 times
				goto sendA;
			}
			perror("didn't get ack yet");
		}
		else if(strncmp(recvData->msgType,"ACK",MSG) != 0){
				if(++d < 5){ //try it 5 times
					goto sendA;
				} 
		}
		
		++index;
	}
	
	fclose(finalFile);
}
 
 //This function finds the number of servers and establishes
 // connections
int makeConnections(char IPlist[LIST][LIST], char portList[LIST][LIST],
					int *socketFD, int *serverList, int min, int *index,
					struct sockaddr_in *serverAddr, int serverCount,
					char *fileName){
	int i, success =0;
	message givData;
	message getData;
	strcpy(givData.msgType, "gFileSize");
	strncpy(givData.buffer,fileName,MAX);
	
	givData.index=1;
	givData.outOf=1;

	int flag2;
	//itterate through servers and try to connect
	for(i=0;  success<min && i < serverCount;++i){ 
	
		if((socketFD[i] = socket(AF_INET, SOCK_DGRAM,0)) <0){
			printf("Can't make socket at server %i\n", i);
		}
		serverAddr[i].sin_family = AF_INET;

		serverAddr[i].sin_port = htons(atoi(portList[i]));

		inet_pton(AF_INET,IPlist[i], &serverAddr[i].sin_addr);
		//flag = 1;
		//flag2 = 0;
		
		printf("AVOUT TO SEND: [%s]\n",givData.buffer);
		givData.serverPort = -1;
		if (setsockopt(socketFD[i], SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
			perror("Error");
		} 
		
			sendAgain:
			printf("Success is: %i   min is: %i   serverCount is : %i  \n",success,min,serverCount);
			//send the maxByte and minByte to the server
			if(sendto(socketFD[i], (void *)&givData, (sizeof(givData)),
				0,(struct sockaddr *) &serverAddr[i],sizeof(serverAddr[i])) < 0 ){
					perror("FAILDED TO SEND\n");
				}

			memset(getData.msgType,0,MSG);
			socklen_t temp = sizeof(getData);
			//try to recv from the server
			if(recvfrom(socketFD[i],&getData,sizeof(getData),0, (struct sockaddr *) &serverAddr[i], &temp) < 0){
				perror("Timeout");
			}
			
			//if we got file not found..
			else if(strstr(getData.msgType,"FNF") != NULL){
				printf("DATA IS: [%s]\n",getData.buffer);
				printf("File was not found. Exiting program\n");
				exit(1);
			}
			//if we got file size back...
			else if(strstr(getData.msgType,"FileSize") != NULL){
				//we now have the size of the file
					fileSize = atoi(getData.buffer);
					printf("Size is [%i]\n",fileSize);
					printf("New port is %i\n",getData.serverPort);
					
				strcpy(givData.msgType, "FileSize");
				
				printf("new sendData buffer is: [%s]\n",givData.msgType);
				serverAddr[i].sin_port = htons(getData.serverPort);	
				printf("success is %i\n",success);
				serverList[success] = i;				
				printf("1.0 i is %i, serverListat i is %i\n",i, serverList[success]);
				success++;
				
			}
			//if the server sent us an ack (server is up, but we already
			// have the file)...
			else if(strstr(getData.msgType,"ACK") != NULL){
				printf("success is %i\n",success);
			
				printf("Got ack from server %i\n",success);
				serverAddr[i].sin_port = htons(getData.serverPort);	
				serverList[success] = i;
				printf("1.00 i is %i, serverListat i is %i\n",i, serverList[success]);
				++success;
				//flag = 0;
			}
			else{// if we didn't get anything, try again
				printf("in else\n");
				flag2++;
				if(flag2 <5)
					goto sendAgain;
				else
					flag2 = 0;
			//DEBUG:printf("recv message is: [%s]\n",getData.msgType);
			
		}
		
	}
	if(success == 0){
		printf("No severs are available\n");
		exit(0);
	}
	return success;
	
}


//CheckArgs() checks for valid arguments
int checkArgs(int *args, char *argv[]){
	//Make sure we have right number of args
	if(*args !=4){ //supposed to be 4
		printf("Usage: <server-info.text> <num-chunks> <fileName>\n");
		exit(1);
	}
	int userReq = atoi(argv[2]);
	if(userReq <=0){
		printf("Can't receive negative chunks\n");	
		exit(1);
	}
	return userReq;
}


//findServers() extracts the servers and ports in the server-list.txt file
int findServers(char IPlist[LIST][LIST], char portList[LIST][LIST], char *fileName){
	int serverCount = 0;
	//crate a buffer and open file for reading
	char buffer[LIST];
	FILE *servertext;
	printf("file name is: [%s]\n",fileName);
	if((servertext=fopen(fileName,"r")) == NULL){
		printf("Can't find server file\n");
		exit(1);
	}
	
	//buffer length
	int len = 0, i = 0;

	//read server info file line by line
	while(fgets(buffer,LIST,servertext)){
		//reset buffer
		
		if((len = strlen(buffer)) == 0)
			break;
		i = 0;
		for(i = 0; i< len;i++){
			if(buffer[i] == ' '){
				break;
			}
		}
			//the first i chars is the IP address
		strncpy(IPlist[serverCount], buffer, i);

		//null char on the last index
		IPlist[serverCount][i]=0;

		
		//copy everything after space (i+1) to the port list
		strncpy(portList[serverCount], &buffer[i+1],len-i-2);
		
		//null char on the last index
		portList[serverCount][len-i-2] = 0;
	
		
		//increment the number of servers
		serverCount++;
	}
	printf("HERE\n");
	char c = buffer[len-1];
	printf("BUFFER AT LEN -1 %c\n",c);
	if(c >= '0' && c <= '9'){
		printf("HERE\n");
		strncat(portList[serverCount-1], &c, 1);
	}
	fclose(servertext);
	return serverCount;
}
