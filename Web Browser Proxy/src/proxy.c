//             Ramzey Ghanaim
//                proxy.c
//                  Lab 4
//     
//
//  This program implements a porxy server
//  that filters websites through a web
//  browser
//  TCP Transpotr layer protocol.
//
#define LIST 1000
#define MAX 1000000
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

int checkArgs(int *args, char *argv[]);
int findBlackList(char bList[LIST][LIST], char *fileName);
void createConn(int port);

int getURL(char *buffer, char *finalURL);

int checkBlackList(char URL[LIST], char blackList[LIST][LIST], int size;);

void logReq(char *buffer, struct sockaddr_in client,char *code, char *size);

char * sendError(int connected, int code);

void appendForward(char *buffer);

int sendReq(char buffer[MAX],char URL[LIST], int connected);

void getCode(char *buffer, char code[3]);

void getLen(char *buffer, char len[100]);

//socket info about the client connecting to this server
struct sockaddr_in client;

//socket info about this server
struct sockaddr_in server;

struct sockaddr_in host_addr;
	
int socketFD;
char *body403 = "<html><h1>403: YOU ARE NOT VERIFIED!</h1></html>\r\n";
char *body501 = "<html><h1>The 501st Error: Not implemented!</h1></html>\r\n";
char *body400 = "<html><h1>400: Bad Request</h1></html>\r\n";
char *connType = "\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n";
char *forward = "Forwarded: for=";
int main(int argc, char* argv[]){
	int port = checkArgs(&argc,argv);
	char blackList[LIST][LIST];
	
	int listCount = findBlackList(blackList,argv[2]);
	int i;
	for(i=0;i<listCount;++i){
		printf("[%s]\n",blackList[i]);
	}
	createConn(port);
	int addrSize = sizeof(struct sockaddr_in);
	int connected;
	printf("Waiting for clients...\n");
	char buffer[MAX];
	//type in terminal: curl localhost:8080/www.reddit.com
	// or  in web browser w/o configuration set up: localhost:8080/www.reddit.com  
	
	
	while((connected = accept(socketFD,
							 (struct sockaddr *)&client,
							 (socklen_t*)&addrSize)) >0){
		pid_t pid = fork(); 
		if(pid == 0){
			close(socketFD);
			printf("Got new client\n");
			memset(&buffer,0,MAX);
			recv(connected,buffer, sizeof(buffer),0);
			//printf("            recieved:   \n[%s]\n",buffer);
			//logReq(buffer, client);
			char URL[LIST];
			char logBuff[MAX];
			strncpy(logBuff,buffer,MAX);
			memset(URL,0,LIST);
			if(getURL(buffer, URL)){ //if get or head request
				//printf("     GOOD GET REQ\n");
				//printf("URL:[%s]\n",URL);
				// if(strstr(URL,"firefox") == 0){
					// exit(0);
				// }
				int block = checkBlackList(URL,blackList,listCount);
				//printf("block is: %i\n",block);
				if(block){
					char *size = sendError(connected, 403);
					logReq(buffer,client,"403",size);
					close(connected);
					printf("Waiting for clients...\n");
					exit(0);
				}
				
				appendForward(buffer);
				//printf("final send buff is: \n%s\n",buffer);
				
				if(sendReq(buffer, URL,connected)){
					char code[3];
					char len[100];
					char tempbuff[MAX];
					//printf("           BUFFER IS \n%s\n",buffer);
					strncpy(tempbuff,buffer,MAX);
					//printf("AT STRCPY\n");
					
					getLen(tempbuff, len);
					if(len == NULL ||strlen(len) == 0){
						strncpy(len,"0",1);
					}
					getCode(tempbuff, code);
					logReq(logBuff,client,code,len);
					
					
					memset(&len,0,100);
					memset(&code,0,3);
				}
				
				close(connected);
			}
			else{
				//printf("HERE\n");
				//printf(" buffer:\n%s\n",buffer);
				char *size = sendError(connected, 501);
				logReq(buffer,client,"501",size);
				close(connected);
			}
			printf("Waiting for clients...\n");

			exit(0);
		}
		
		close(connected);
	}
	
	return 0;
}

//          getLen()
// This function takes in the HTTP header
// and extracts the length of the data buffer
// by looking for the "Content-Length: " header
void getLen(char *buffer, char len[100]){
	//printf("in the function\n");
	//printf("BUFFER IS: [%s]\n",buffer);
	char find[] = "\n";
	char *begin = strstr(buffer,"Content-Length");	
	//int len2 = strnlen(buffer,MAX);
	//buffer[len2] = 0;
	begin += 16;
	char *end = strstr(begin, find);
	
	strncpy(len,begin, end+1-begin);
	
	//   printf("len is: %s",len);
}

//          getCode()
// This function takes in the HTTP header
// and extracts the status code of the data buffer
// by looking for the code int he first line of
// the header.
void getCode(char *buffer, char code[3]){
	//printf("getting code\n");
	char *token;
    char *s = " ";
	int i =0;
   /* get the first token */
   token = strtok(buffer, s);
   while(token != NULL){
	  // printf("%i:%s\n",i,token);
	   ++i;
	   if(i == 2){
		   strncpy(code,token,3);
		    
		   break;
	   }
	   token = strtok(NULL, s);
	
   }
   //printf("code INSIDE: %s\n",code);
   
}


//          sendReq()
// This function takes in the request message from the browser
// and sends it to the server destination. Lastly, this function
// retrieves the response from the server
int sendReq(char buffer[MAX],char URL[LIST], int connected){
	struct hostent* host;
//Get IP address from URL
	host=gethostbyname(URL);
	struct in_addr **address;
	if(host == NULL){ //send error 400 if bad request
		perror("HOST IS NULL");
		char *size = sendError(connected, 400);
		logReq(buffer,client,"400",size);
		close(connected);
		printf("Waiting for clients...\n");
		exit(0);
	}
	int success = 0;
	int port = 80;
	host_addr.sin_port=htons(port);
	host_addr.sin_family=AF_INET;
	//format IP address
	address = (struct in_addr **) host->h_addr_list;
	
	char dest[100];
	strcpy(dest,inet_ntoa(*address[0]));
	//address = (struct in_addr *) (host->h_addr);
	printf("connecting to: %s\n",dest);
	inet_pton(AF_INET,dest,&(host_addr.sin_addr));
	
	//make a socket and connect
	int sockfd1=socket(AF_INET,SOCK_STREAM,0);
	int connect2=connect(sockfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));

	if(connect2<0)
		printf("Error in connecting to remote server\n");
	
	//send the data to the server
	if(send(sockfd1,buffer,strlen(buffer),0) < 0){
		printf("Failed to send\n");
	}
	//printf("ABOUT TO RECV\n");
	int n;
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	char tempBuff[MAX];
	setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	//Get data from server and forward to browser until
	//we don't have any data left
	do{
	if((n=recv(sockfd1,tempBuff,MAX,0))< 0){
		//printf("Can't recv\n");
		break;
	}
	else{
		success =1;
	//DEBUG: printf("           BUFFER IS \n%s\n",tempBuff);
		send(connected,tempBuff,n,0);
		strcpy(buffer,tempBuff);
	}
	}while( (n <=MAX && n > 0 ));
	
	//DEBUG:printf("succes is :%i\n",success);
	//end the connection with the browser
	close(sockfd1);
	return success;
	//use getaddrinfo()
	
}

//          appendForward()
// This function takes in the HTTP header
// and adds a forward message to the second
// line of the headder to tell the server
// a proxy is being used
void appendForward(char *buffer){
	//printf("about to forward buffer: \n%s",buffer);
	char sendBuff[MAX];
	char *first = strstr(buffer,"\n");
	strncpy(sendBuff,buffer,first+1-buffer);
	
	// strncpy(sendBuff,buffer,strnlen(buffer,MAX)-2); 
	 strncat(sendBuff,forward, MAX);
	 char *clientIP = inet_ntoa(client.sin_addr);
	 strncat(sendBuff,clientIP, MAX);
	 strncat(sendBuff,"; proto=http; by=",MAX);
	 char *proxyIP = inet_ntoa(client.sin_addr);
	 strncat(sendBuff,proxyIP,MAX);
	 
	 strcat(sendBuff,first);
	//printf("final send buff is: \n%s\n",sendBuff);
	
	
}

//          sendError()
// This function takes sends the error message
// to the browser for 501, 403 and 400 errors.
// Lastly, the error is logged.
char * sendError(int connected, int code){
	char sendBuff[MAX];
	char len[LIST];
	memset(&len,0,LIST);
	char codeS[3];
	int bodyLen;
	char msg[LIST];
	sprintf(codeS,"%d",code);
	char *body;
	//Assign error message based
	// off of error code
	if(code == 403){
		bodyLen = strlen(body403);
		strcpy(msg," Forbidden\r\n");
		body = body403;
	}
	else if(code == 501){
		bodyLen = strlen(body501);
		strcpy(msg," Not Implemented\r\n");
		body = body501;
	}
	else if(code == 400){
		bodyLen = strlen(body400);
		strcpy(msg," Bad Request\r\n");
		body = body400;
	}
	
	//Make the header buffer
	sprintf(len,"%d",bodyLen);
	
	//append request type
	strncpy(sendBuff,"HTTP/1.0 ",MAX);
	//append status code
	strncat(sendBuff,codeS,3);
	
	strncat(sendBuff,msg,(int)strnlen(msg,LIST));
	strcat(sendBuff,"Accept-Ranges: bytes\r\nContent-Length: ");
	
	strncat(sendBuff,len,(int) strlen(len));
	strncat(sendBuff,connType,(int) strlen(connType));
	
	//attach the body
	strncat(sendBuff,body,bodyLen);
	send(connected,sendBuff,strlen(sendBuff),0);
	//DEBUG:printf("about to send buffer: %s",body);
	memset(&sendBuff,0,MAX);
	memset(&codeS,0,3);
	char *ret = len;
	return ret;
}

//          logReq()
// This function takes in the HTTP header
// and extracts the first line, and writes it
// to the log file with the current date and time
// status code and size of the data
void logReq(char *buffer,struct sockaddr_in client, char *code, char *size){
	if(strlen(buffer) <= 0){
		return;
	}
	char firstLine[MAX];
	int len =0;
	while(buffer[len] != '\n'){
		
		firstLine[len] = buffer[len];
		++len;
	}
	
//	printf("first line is: [%s]\n",firstLine);
	FILE *fileptr;
	fileptr = fopen("access.log","a+");
	char buff[20];
    struct tm *sTm;

    time_t now = time (0);
    sTm = gmtime (&now);

    strftime (buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S", sTm);
	fwrite(buff,1,strnlen(buff,20),fileptr);
	fwrite(" ",1,1,fileptr);
	char *ip = inet_ntoa(client.sin_addr);
	fwrite(ip,1,strlen(ip),fileptr);
	fwrite(" \"",1,2,fileptr);
	//firstLine[len-1] = 0;
	fwrite(firstLine,1,len-1,fileptr);
	fwrite("\" ",1,2,fileptr);
	fwrite(code,1,strlen(code),fileptr);
	fwrite(" ",1,1,fileptr);
	fwrite(size,1,strlen(size),fileptr);
	if(firstLine[0] != 'G' &&firstLine[1] !='E'&&firstLine[2]!='T'){	
		fwrite("\n",1,1,fileptr);
	}
	fclose(fileptr);
}

//          checkBlackList()
// This function takes in the URL the user wants
// to go to, and the blackList  and compares the two.
// This function is robust enough to check through 
// urls with and without www. and http//
int checkBlackList(char URL[LIST],char blackList[LIST][LIST], int size){
	int i =0, e =0, flag = 1;
	int len = strnlen(URL,LIST);
	
	if(len<4){
		printf("Bad url\n");
		exit(1);
	}
	//remove www.
	for(;i<len;++i){
		if(URL[i]=='w'&&URL[i+1]=='w'&&URL[i+2]=='w'&&URL[i+3] =='.'){
			i+=4;
			flag=0;
			break;
		}
	}
	//printf("i is: %i, URL len is: %i\n",i, len);
	
	//check for a URL match with the blackList
	char temp2[LIST];
	for(e=0;e<size;++e){
		if(flag){
			strncpy(temp2,&blackList[e][4],strnlen(blackList[e],LIST));
		//	printf("temp 2 is: [%s]\n",temp2);
			//if a match occurs send 1
			if(strcmp(URL,temp2)==0){ 
				return 1;
			}
		}
		//if a match occurs send 1
		else if(strcmp(URL,blackList[e]) == 0){
			return 1;
		}
	}
	
	return 0;
}

//          getURL()
// This function takes in the HTTP header
// and extracts the URL the user wants to 
// go to. The function returns 1 if a GET
// or HEAD request is gotten.
// Otherwise, 0 is returned
int getURL(char *buffer, char *finalURL){
	int i;
	int e;
	int getOrHead = 0;
	//int getFlag = 0;
	for(i=0;buffer[i] != '\n';++i){
		if(buffer[i] == 'G' &&buffer[i+1] =='E' && buffer[i+2] =='T'){
			getOrHead =1;
			if(buffer[i+4]=='/'){
				i+=5;
				//getFlag=1;
				
				printf("buffer at i is: %c\n", buffer[i]);
			}
			else{
				i+=4;
				//printf("ELSE buffer at i is:%c\n", buffer[i]);
			}
			if(buffer[i]=='h'&&buffer[i+1]=='t'&&buffer[i+2]=='t'&&buffer[i+3]=='p'&&
				buffer[i+4]==':'&&buffer[i+5]=='/'&&buffer[i+6]=='/'){
				i+=7;
			}else if(buffer[i]=='H'&&buffer[i+1]=='T'&&buffer[i+2]=='T'&&buffer[i+3]=='P'&&
				buffer[i+4]==':'&&buffer[i+5]=='/'&&buffer[i+6]=='/'){
				i+=7;
			}
			
			break;
		}
		else if(buffer[i] == 'H' &&buffer[i+1] =='E' && buffer[i+2] =='A' && buffer[i+3] == 'D'){
			getOrHead = 1;
			//look for head msg
			//printf("   dsklafldsak fs HERE\n");
			char *begin = strstr(buffer,"Host:");
			char *end = strstr(begin, "\r");
			begin+=6;
			
			strncpy(finalURL,begin,end - begin);
			return getOrHead;
		}
	}
	
	for(e=i+1;buffer[e]!='\n';++e){
		if(buffer[e] == 'H' &&buffer[e+1] =='T' && buffer[e+2] =='T'
			&&buffer[e+3] == 'P'){
			if(buffer[e-2] == '/'){
				--e;
			}
			break;
		}
	}
	
		//copy everything after space (i+1) to the port list
	
	strncpy(finalURL, &buffer[i],e-i-1);
	
	for(i = 0; i< strlen(finalURL);++i){
		if(finalURL[i] == '/'){
			finalURL[i] = 0;
			break;
		}
	}
	
	return getOrHead;
}

//          createConn()
// This function establishes a connection with
// the browser based off a provided port number
void createConn(int port){
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

//              findBlackList()
// This function extracts the servers and ports 
// in the server-list.txt file
int findBlackList(char bList[LIST][LIST], char *fileName){
	int listCount = 0;
	//crate a buffer and open file for reading 
	char buffer[LIST];
	FILE *servertext;
	printf("file name is: [%s]\n",fileName);
	if((servertext=fopen(fileName,"r")) == NULL){
		printf("Can't find server file\n");
		exit(1);
	}
	
	//buffer length
	int len = 0;

	//read server info file line by line
	while(fgets(buffer,LIST,servertext)){
		//reset buffer
		
		if((len = strlen(buffer)) <=1)
			break;
		
		
		strncpy(bList[listCount], buffer,len-1);
		//printf("[%s]\n",bList[listCount]);
		
		listCount++;
	}
	char c = buffer[len-1];
	if(c >= 'a' && c <= 'z'){
		printf("HERE\n");
		strncat(bList[listCount-1], &c, 1);
	}
	fclose(servertext);
	return listCount;
}

//            CheckArgs() 
// This function checks for valid arguments
int checkArgs(int *args, char *argv[]){
	//Make sure we have right number of args
	if(*args !=3){ //supposed to be 4
		printf("Usage: <port> <frobidden.txt>\n");
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
		printf("invalid port\n");	
		exit(1);
	}
	return port;
}
