//         Ramzey Ghanaim
//         myclient.c
//
// This program is used as a leecher to 
// downland a file from multiple servers/seeders
// This program is prone to server crashes.
//
// Apologies about the lack of code organization/functions.
// Whenever I put my code into functions my servers and clients
// would not connect. The TA tried to help me for over an hour
// and could not debug the issue. Copying and pasting my code from
// my function to main() seemed to fix all my connection issues...
//
#define MAX 1280
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

typedef struct{
	int minByte;
	int maxByte;
	int socketFD;
	int *error;
	char *buffer;
	int index;
}data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* getData(void* threadData);
int main(int argc, char* argv[]){
	
	//Make sure we have right number of args
	if(argc !=4){ //supposed to be 4
		printf("Usage: <server-info.text> <num-connections> <fileName>\n");
		exit(1);
	}
	//keep track of number of servers available
	int serverCount = 0;

	//crate a buffer and open file for reading
	char buffer[MAX+1];
	FILE *servertext;
	if((servertext=fopen(argv[1],"r")) == NULL){
		printf("Can't find server file\n");
		exit(1);
	}
	
	//create a list of ports and associated IPs
	char IPlist[MAX][MAX], portList[MAX][MAX];

	//buffer length
	int len = 0, i = 0;

	//read server info file line by line
	while(fgets(buffer,MAX,servertext)){
		if((len = strnlen(buffer,MAX)) == 0)
			break;
		i = 0;
		for(i = len-1; i>=0;i--){
			if(buffer[i] == ' '){
				break;
			}
		}

		//the first i chars is the IP address
		strncpy(IPlist[serverCount], buffer, i);

		//null char on the last index
		IPlist[serverCount][i]=0;

		printf("IP: [%s]",IPlist[serverCount]);
		
		//copy everything after space (i+1) to the port list
		strncpy(portList[serverCount], &buffer[i+1],len-i-2);
		
		//null char on the last index
		portList[serverCount][len-i-2] = 0;

		printf("   PORT [%s]\n",portList[serverCount]);
	
		//reset buffer
		memset(buffer, 0, sizeof(buffer));
		//increment the number of servers
		serverCount++;
	}
	
	//get number of requested servers
	int userReq = atoi(argv[2]);
	
	//calculate the min between server in the text file
	// and servers requested by the user
	int min = (serverCount) <= (userReq) ? (serverCount) : (userReq);
	
	//we are done with the server-info file
	fclose(servertext);

	printf("we found %i servers \n", serverCount);
//----------connect to server now ----------------------

	//initalizations
	int socketFD[min];
	int serverList[serverCount];
	struct sockaddr_in serverAddr[min];
	int index = 0, flag = 0;
	memset(socketFD, -5, serverCount);
	
	//make conniections
	if(userReq < 1){
		printf("Can't connect to %i servers\n",userReq);
		exit(1);
	}
getconn:	
	//itterate through servers and try to connect
	for(i=0;i<userReq && index<serverCount;++i){
		if((socketFD[i] = socket(AF_INET, SOCK_STREAM,0)) <0){
			printf("Can't make socket at server %i\n", i);
		}
		
		serverAddr[i].sin_family = AF_INET;

		serverAddr[i].sin_port = htons(atoi(portList[index]));

		serverAddr[i].sin_addr.s_addr=inet_addr(IPlist[index]);
		//DEBUG: printf("Trying: port: %s\n",portList[index]);
		while(index <serverCount && (serverList[index]=connect(socketFD[i], 
									(struct sockaddr *) &serverAddr[i], 
									sizeof(serverAddr[i])) )< 0){
			//if server didn't connect, change the IP and port and try again
			++index;
			
		//DEBUG:printf("----SERVER LIST AT INDEX: %i is %i \n",index, serverList[index]);
			if(index >= serverCount){
				
				flag = 1;
				break;
			}
			serverAddr[i].sin_port = htons(atoi(portList[index]));
			
			serverAddr[i].sin_addr.s_addr=inet_addr(IPlist[index]);
		//DEBUG:printf("^^ failed. Trying: port: %s\n",portList[index]);
		
		}
		if(flag == 1){
			break;
		}
		if(index >= serverCount){
			++i;
			break;
		}
		++index;
	}

//---------done connecting----------------------------------------------------
	int size = 0;
	int successConn = i;
	int numOfFailures = 0;
	
	//DEBUG: printf ("DONE MAKEING %i CONNECTIONS    index is %i\n", successConn, index);
	
	//calculate number of failures
	if(successConn < min){
		numOfFailures += (min - successConn);
		}
	if(numOfFailures == min){
		if(numOfFailures==min-1 && serverList[serverCount-1] == 0){
			++numOfFailures;
		}
		//exit if all servers fail to connect
		printf("Num of failures is %i\n",numOfFailures);
		printf("Couldn't connect to any servers\n");
		exit(1);
	}
	
	//keep track of indexes of all successful servers
	// in the successServerIndex array
	int successServerIndex[successConn];
	int temp = 0;
	for(i = 0; i<successConn;++i){
		printf("SERVer list at %i, is %i\n",i, serverList[i]);
		successServerIndex[temp] = i;
			
		//DEBUG: printf("succesfful server index is: %i\n",temp);
		//DEBUG: printf("%i connect status: %i      socketfd: %i\n",i, serverList[i],socketFD[successServerIndex[temp]]);
		++temp;
			
	}
	

	//user command
	char input[MAX+1]; 
	//result from server
	char result[MAX+1];
	
	memset(input,0,MAX+1);
	memset(result,0,MAX+1);
	printf("Number of connections is: %i\n",successConn);	
	
	//ask for file size
	strcpy(input, "getFileSizeF ");
	strncat(input, argv[3], MAX);
	
	//attach end of message char
	strncat(input, "@",MAX+1);
	int strnLen = strnlen(input, MAX+1);
	input[strnLen] = 0;
	
	
	//DEBUG: printf("ABOUT TO SEND to server: %i, at fd: %i\n",successServerIndex[0],socketFD[successServerIndex[0]]);
	
	//send the file name to server to get
	// file size. we iterate through all servers
	// to account for failing servers
	for(i =0; i<successConn;++i){
		if((send(socketFD[successServerIndex[i]], &input, strnLen, 0)) < 0)
			printf("%s\n", "unable to send");
		else{ //break when we successfully get the file size
			if(recv(socketFD[successServerIndex[i]],buffer,sizeof(buffer),0) > 0){
				len = strnlen(buffer,MAX+1);
				//printf("BUFFER IS [%s]  size: %i \n", buffer, len);
				if(buffer[len-1] == '@'){
					buffer[len-1] = 0;
					break;
				}
			}
			else
				printf("Did not get size [%s]\n",buffer);
		}
			
	}
	
	
	memset(input,0,MAX+1);
	//simply send file name to all
	//other servers.
	//This message tells server to only
	//open a file pointer, don't send
	//file size back
	strcpy(input, "getFileSize ");
	strncat(input, argv[3], MAX);
	strncat(input,"@",MAX+1);
	strnLen = strnlen(input, MAX+1);
	input[strnLen] =  0;

	//send to all remaining servers
	++i;
	for(; i<successConn;++i){
		
		send(socketFD[successServerIndex[i]], &input, strnLen,0);
	}
	
	//check to see if the response from the first server
	// was "file not found"
	if(strstr(buffer, "File Not Found@")){
		printf("File does not exist\n");
		exit(1);
	}
	
	//check to see if the response from the first
	//server contained the file size
	if(strstr(buffer, "file size ") != NULL){
		char fileSize[MAX]; 
		
		//copy everything after the message "file size "
		strncpy(fileSize, &buffer[10],len-1);
		//DEBUG: printf("FILE Zize: %s\n", fileSize);
		
		//we now have the size of the file
		size = atoi(fileSize) / successConn;
		//DEBUG:printf("Size is [%i]\n",size);
	

		}
		else{ //if we failed to get the file size
			
			//if we can still access more servers
			//go back to making connections
			if( index < serverCount){
				goto getconn;
			}
			
			//otherwise tell user we lost all connections
			//and exit
			printf("File not found or lost connection to all servers\n");
			exit(1);
		}
		
		//calculate the remainder and total size of the
		//file for verification of my calculation
		int remainder = size % successConn;
		int totalSize = (size * successConn)+remainder;

		//DEBUG: printf("TOTAL: %i\n", totalSize);
		
		//use my structure called "data" to 
		//keep track of all information
		//each thread needs
		data threadData[successConn];
		int nextMin=0;
		
		//create final file 
		FILE *finalFile;
		finalFile=fopen("newFile.txt","w");
		
//--------------------set up thread data structure----------------------------
//   The goal is to itterate through for each server and assign it 
//   a specific min and max byte, along with an error flag, and index
//   and a buffer to store a fragment of the file in
	
	//size of data we want to send each server
	int dataSize=0; 
	
	//Don't itterate thorugh last file, we simply
	//put the remainder + dataSize here
	for(i=0;i<successConn-1;++i){
			if(i==0){
				threadData[i].minByte = 0;
				threadData[i].maxByte = size;
				printf("thread %i: Min = %i     thread Max= %i\n", i, threadData[i].minByte, threadData[i].maxByte);
				dataSize = (threadData[i].maxByte-threadData[i].minByte)+2;
			}
			else{
				threadData[i].minByte = nextMin;
				threadData[i].maxByte = nextMin+size-1;
				//DEBUG: printf("thread %i: Min = %i   thread Max = %i\n", i, threadData[i].minByte, threadData[i].maxByte);

			}
				//DEBUG: printf("index = %i,   socket fd is: %i\n", successServerIndex[i],socketFD[successServerIndex[i]]);
				threadData[i].socketFD = socketFD[successServerIndex[i]];
				nextMin = threadData[i].maxByte +1;
				threadData[i].error = malloc(sizeof(int) *1);
				threadData[i].error = 0;
				threadData[i].index = i;
				threadData[i].buffer = malloc(sizeof(char) *((threadData[i].maxByte-threadData[i].minByte)+2));
	}
	
	//get size + remainder data from last server
	threadData[i].minByte = nextMin;
	threadData[i].maxByte = nextMin+size+remainder-1;
	threadData[i].socketFD = socketFD[successServerIndex[i]];
	threadData[i].error = malloc(sizeof(int)*1);
	threadData[i].error = 0;
	threadData[i].buffer = malloc(sizeof(char) *(threadData[i].maxByte-threadData[i].minByte+2));

	//DEBUG: printf("thread %i: Min = %i   thread Max = %i\n", i, threadData[i].minByte, threadData[i].maxByte);

//-----Done settingup thread data structure---------------------------------------

//-------------------pthread stuff------------------------------------------------------------
	
	//make threads and fire them
	pthread_t t[successConn];
	for(i = 0; i<successConn;i++){
		if(pthread_create(&t[i],NULL,getData, &threadData[i])){
			printf("error making thread %i\n",i);
		}
		//DEBUG: printf("made a thread\n");
	}
	
	//wait for all threads to join to check for success
	for(i = 0; i< successConn;++i){
		pthread_join(t[i],NULL);
	}
//^^^^^^^^^^^^^^^^^^^^^Done with Pthread stuff ^^^^^^^^^^^^^^^^^^^^^^^^^


//------------------Now we check for errors-----------------------------------
	//determine number of successful downloads
	int success = 0;
	
	//determine number of failed downlands
	int error = 0;
	
	//keep track of good servers
	int good[successConn];
	
	//keep track of bad servers
	int bad[successConn];
	
	index = 0;
	
	//allocate a finalResult buffer to store
	//data that came successfully
	char **finalResult = (char **) malloc(sizeof(char) * dataSize);
	 for(i=0;i<successConn;++i){
		 finalResult[i] = (char *) malloc(sizeof(char)* dataSize);
	 }

		for(i=0;i<successConn;++i){
		
			//printf("error is %i\n",threadData[i].error);
			
			//keep track of good servers
			if(threadData[i].error == 0){	
				good[success] = i;
				++success;
			}
			else{
				//keep track of bad servers
				printf("ERROR. Getting new server\n");
				bad[error] = i;
				++error;
			}
		}
//^^^^^^^^^^^^^^Done checking for errors^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//-----------------We handle errors and fire new threads here-------------
	while(error != 0){ // we have errors
	//printf("ERROR IS: %i\n", error);
		for(i=0;i<error;++i){
			//if only one server is available
			if(index ==success-1){
				//assign a good socket FD to the bad threadData
				threadData[bad[i]].socketFD = threadData[good[index]].socketFD;
				
				//fire a thread and wait for it to complete
				//since there is only 1 good server, we need to wait
				//so data isn't corrupted or overwritten with multiple
				//threads working at the same time
			//	printf("HERE 1 : %i\n",index);
				pthread_create(&t[good[index]],NULL,getData,&threadData[bad[i]]);
				pthread_join(t[good[index]],NULL);
				if(threadData[bad[i]].error == 0){
					--error;
				}
			}
			else{//more servers are available
				//fire threads if there are 
				//printf("HERE: %i \n", index);
				threadData[bad[i]].socketFD = threadData[good[index]].socketFD;
				pthread_create(&t[good[index]],NULL,getData,&threadData[bad[i]]);
				pthread_join(t[good[index]],NULL);
				++index;
				
				//exit if all servers are bad
				if(error >=successConn){
					printf("Lost all servers\n");
					exit(1);
				}
			}
		}
		
		//wait for all remaing threads
		for(i = 0; i< successConn;++i){
			pthread_join(t[i],NULL);
		}
	}
//^^^^^^^^^^^^^^^^^ALL done with handling errors^^^^^^^^^^^^^^^^^^^^^^^^^^^


//--------------Write data to final file---------------------------------
	//At this point all data was gotten
	//successfully. we can write it to the
	//file in order
	for(i=0;i<successConn;++i){
			if(threadData[i].error == 0){
				int buffSize = threadData[i].maxByte - threadData[i].minByte+1;
				if(threadData[i].maxByte == totalSize){
					buffSize --;
				}
				//write data to the file
				fwrite(threadData[i].buffer,1,strlen(threadData[i].buffer),finalFile);
				
				good[success] = i;
				++success;
			}
			else{
				//printf("ERROR. Getting new server\n");
				bad[error] = i;
				++error;
			}
		}
//^^^^^^^^^^^^^ Done writing data to final file^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		
	//check file size
	fseek(finalFile,0,SEEK_END);
	int finalSize = ftell(finalFile);

	if(finalSize== 0){
		printf("Could not get file\n");
	}

	else if(finalSize ==totalSize){
		printf("File successfully downloaded to %s\n",argv[3]);
	}
	

	//close and free everything 
	fclose(finalFile);	
	for(i=0;i<min;i++){
			close(socketFD[i]);	
	}

	for(i = 0; i<successConn;++i){
		free(threadData[i].error);
		free(threadData[i].buffer);
		free(finalResult[i]);
	}
	free(finalResult);

	pthread_mutex_destroy(&mutex);
	return 0;
}// end main







//                      getData()
// @param: tdata - the structure that contains all the data
//                 needed to ask a server for data from min
//                 to max
//
//
// getData() sets the message to the proper protocol for the
// server to read and send us the appropriate data.
// The data is saved in the buffer of the struct so it can
// be placed in the finalResult buffer and ultimately the
// final file.
void* getData(void* tdata){
	
//	pthread_mutex_lock(&mutex);

	//set up the message
	char sendBuffer[MAX+1];
	char temp[MAX];
	strncpy(sendBuffer, "dataSize ",MAX);
	data *threadData = (data*) tdata;
	sprintf(temp, "%d", (*threadData).minByte);
	strncat(sendBuffer,temp,MAX);
	strncat(sendBuffer," ", MAX);
	memset(temp,0,MAX);
	sprintf(temp,"%d", (*threadData).maxByte);
	strncat(sendBuffer,temp,MAX);
	strncat(sendBuffer,"@", MAX+1);
	//printf("about to send [%s]\n",sendBuffer);
	int len = strnlen(sendBuffer, MAX+1);
	sendBuffer[len] = 0;
	
	//send the message
	if(send((*threadData).socketFD, &sendBuffer, strnlen(sendBuffer, MAX+1),0)<0){
		printf("Failed to send s\n");
	}
	else{
		//DEBUG: printf("Sent successfully\n");
	}
	memset(sendBuffer,0,MAX+1);
	
	//calculate expected size of the buffer
	int buffSize = (*threadData).maxByte - (*threadData).minByte +2;

	if(recv((*threadData).socketFD,(*threadData).buffer,buffSize,0) <=0){
		(*threadData).error = (int *) 1;
		printf("can't recv\n");
	}
	else{
	//	printf("---size of buffer is: %zu\n",strnlen((*threadData).buffer,buffSize)); 
		//printf("====buffer is:====\n [%s]\n",(*threadData).buffer);
		(*threadData).error = (int *) 0;
	}
//	pthread_mutex_unlock(&mutex);
	return NULL;
}
