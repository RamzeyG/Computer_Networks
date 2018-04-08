//         Ramzey Ghanaim
//            server.c
//             Lab 2 
//
//  This server is used as a seeder to seed
//  a specified ammount of data from a text file
//  to client
//  
//


#define MAX 1280//128000
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


FILE *fileptr;

void sendData(int maxByte, int minByte, int connected, FILE *fileptr);
int fileLen; //size of the file
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
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	//check for errors
	if(socketFD <0){
		printf("Error: Can't make a socket\n");
		exit(1);
	}

	
	//set up attributes
	server.sin_family = AF_INET; //set connection to TCP
	server.sin_port = htons(atoi(argv[1])); //assigns port number
	server.sin_addr.s_addr = INADDR_ANY; //sets addr to any interface
	
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
	
	//make variables
	int connected; //connection status
	int addrSize = sizeof(struct sockaddr_in); //size of sockaddr_in
	char request[MAX+1]; //buffer to read in client messages
	char fileName[MAX+1]; //string to store file name in

	
    char msg[MAX+1];//temp buffer to send messages to client
	char tempData[MAX]; //temp buffer to store data when reading file
	int strLen;//used to store lengths of messages

	int minByte;//min byte we need to send to cient
	int maxByte;// max byte we send to the client
	int i, index; //increment variables
	
	printf("waiting for clients...\n");
	while((connected = accept(socketFD,
							   (struct sockaddr *)&client,
							   (socklen_t*)&addrSize)) >0)
	{ //while traffic is coming
		printf("Welcome client, you've been expected!\n");
		
		int flag = 0;
		
		printf("waiting to recv\n");
		
		//reset request buffer
		memset(request,0,sizeof(request));
		
		//what to do when recving data
		while(recv(connected,request, sizeof(request),0) > 0){
			
			//look for @ end char of the string
			strLen = strnlen(request,MAX+1);
			//DEBUG: printf("BUFFER IS: [%s]\n",request);
			if(request[strLen-1] == '@'){
				flag = 1;
			}
			
			//if file size request
			if (strstr(request, "getFileSize") != NULL && flag) {
				memset(fileName, 0, MAX);
				strLen = strnlen(request,MAX+1);
				index= 0;
				for(i=strLen-2;i>11;--i){ //i = 12
					if(request[i] == ' ')
						break;
					//fileName[index] = request[i];
					++index;
				}

				//extract file name from message
				strncpy(fileName, &request[i+1], strLen-i-2);
				
				//DEBUG: printf("FILE NAME IS: [%s]\n", fileName);
				
				//open file
				fileptr = fopen(fileName,"r");


				//if file was not found
				if(fileptr == NULL) {
					//prompt client
					printf("FILE NOT FOUND\n");
					write(connected,"File Not Found@", MAX+1);
				}
			   else { //file found
						/// if user request is simply send the file name
						if(strstr(request,"getFileSizeF ") == NULL){
							//if exit flag is on, exit
							if(exitFlag){
								printf("exit flag is on\n");
								exit(0);
							}
						}
						else{//otherwie get file size and send it to user
						
						//if exit flag is on, exit before sending info back to server
							if(exitFlag){
								printf("exit flag is on\n");
								exit(0);
							}
							//get to end of file 
							fseek(fileptr,0,SEEK_END);
							
							//pointer location is now the 
							//size of the file
							fileLen = ftell(fileptr);
							
							//put pointer back to beginning
							rewind(fileptr);
	
							//get message ready to send
							strncat(msg,"file size ", MAX);
							sprintf(tempData,"%d",fileLen);
							strncat(msg,tempData,MAX);
							printf("file size: %i bytes\n", fileLen);
							strncat(msg, "@",MAX+1);

						    //DEBUG:printf("ABOUT TO SEND [%s]\n",msg);
							
							//send message
							if(write(connected,msg, MAX+1) > 0){
								printf("Sent successfully\n");
							}
							else{
								printf("failed to send\n");
							}
						}

			   }
			   //fclose(fileptr);
			}


			//if user provides data size: max and min
			//we want to format data and send 
			else if(strstr(request, "dataSize ") != NULL){
				//Format the min and max into numbers
				index =0;
				memset(tempData, 0, MAX);
				strLen = strnlen(request, MAX+1);
				for(i=9;i<strLen;++i){
					if(request[i] == ' ')
						break;
					tempData[index] = request[i];
					++index;
				}
				//DEBUG: printf("min Is: [%s]\n",tempData);
				
				//finally extracted minByte
				minByte = atoi(tempData);
				
				memset(tempData,0,MAX);
				
				//extract maxByte
				++i;
				index = 0;
				for(;i<strLen;++i){
					if(request[i] == '@'){
						break;
					}
					tempData[index] = request[i];
					++index;
				}
				//DEBUG:printf("max Is: [%s]\n",tempData);
				
				//finally extracted maxByte
				maxByte = atoi(tempData);
				
				//call sendData() to get the data and send to
				//appropriate connected client
				sendData( maxByte, minByte, connected, fileptr);
				
				//DEBUG: printf("file pointer ends up at: %ld \n", ftell(fileptr));

				
			}
			
		//reset all buffers
		memset(request,0,MAX+1);
		memset(tempData,0,MAX);
		memset(msg,0,MAX+1);
		printf("Waiting for next message\n");
		}
				//close file pointer when we are done
				//fclose(fileptr);
	}	
		
		
		
	return 0;
}//end main()


//                      sendData()
// @param: maxByte - maximum byte we want to read
// @param: minByte - the first byte we want to read
// @param: connected - the client we want to send data to
// @param: fileptr - file pointer used to increment through
//                   and read data from the file
//
//
// sendData() sets the file pointer to the byte location specified by
// minByte. We read in data up until maxByte.
// Lastly, we send the data to the connected client
void sendData(int maxByte, int minByte, int connected, FILE *fileptr){
	
	//make appropriate buffer size
	int buffSize = maxByte - minByte+1;
	
	//go to the minByte location
	fseek(fileptr,minByte,SEEK_SET);
	if(maxByte == fileLen){
		buffSize --;
	}
	//DEBUG: printf("pointer location BEFORE: %ld bytes", ftell(fileptr));
	
	//buffer used to store data
	char *buffer[buffSize];
	memset(buffer,0,buffSize);
	
	//read data from the file pointer location
	//up until the max size of the buffer -1
	fread(buffer, 1, buffSize, fileptr);
	
	
	//DEBUG: printf("pointer location AFTER: %ld bytes", ftell(fileptr));

	//DEBUG: printf("ABOUT TO SEND: [%s]\n", buffer);

	
	//send data to client
	write(connected, buffer, buffSize);
} //end sendData()
