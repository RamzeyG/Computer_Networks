//             udp_helper.c
//
// This file contains all my helper functions to help the server and client
// files.
//
// @author: Ramzey Ghanaim

# include "primitives.h"

//Denine len()
#define len(array) (sizeof((array))/sizeof((array)[0]))

// Database
char phoneNum[25][20]; char tech[25][3]; char paid[25][3];


// Server connection info Structure
typedef struct{
	struct sockaddr_in client;
    struct sockaddr_in server;
    int socketFD;
}connectInfo;

typedef enum boolean {false, true} bool;


//    Packet Struct
// This struct contains all the info of a packet
typedef struct{
	uint16_t startOfPacketID;
	uint16_t packetTypeID;
	uint16_t endOfPacketID;
	// uint16_t rejectSubCode;

	uint8_t clientID;
	uint8_t segmentNum;
	uint8_t payloadLen;
	uint8_t payload[LEN_OF_PAYLOAD];
	
	// Payload:
	uint32_t phoneNumber;
	uint8_t technology;

	bool stringPayload;
	 
}packetInfo;



// Timeout Variable {sec, usec}
struct timeval tv={TIMEOUT,0};


//            handleTimeout()
//
// Helper function that handles the server timeout
//
// @param: myConnection - connectInfo - current conneciton info
// @param: timeOutCtr - int - time out counter
//
void handleTimeout(connectInfo myConnection, int timeOutCtr){
	printf("Handling Timeout\n");
	if(++timeOutCtr > 3){
		// timeout more than 3 times
	}

}


//             makeConn()
//
// Function that creates the server-side of the connection
//
// @param: port - int - port the user wants to use
// @param: endpoint - char * - Specifies if we want to make a client or server
//                    connection
//
// @return: myConnection - connectInfo - the new connection info that was 
//                         just created.
//
connectInfo makeConn(int port, char *endpoint){

    connectInfo myConnection;

    myConnection.socketFD = socket(AF_INET, SOCK_DGRAM, 0);


	//set up attributes
	myConnection.server.sin_family = AF_INET; //set connection to TCP
	myConnection.server.sin_port = htons(port); //assigns port number
	myConnection.server.sin_addr.s_addr = INADDR_ANY; //sets addr to any interface


	// Only the server binds...
	if(strncmp(endpoint, "server", 6) == 0){

		//bind server information to socket
		if(bind(myConnection.socketFD,(struct sockaddr *) &(myConnection.server),
					sizeof(myConnection.server)) <0){
			perror("can't bind\n");
			close(myConnection.socketFD);
			printf("Error, could not bind properly\n");
			exit(1);
		}

	}
	
    return myConnection;
}


//           argChecker()
//
// This function checks the args provided into main() by the user. Ther function
// will exit out of the program if the arguments are incompatable.
// 
// @param: argc - int - number of arguments
// @param: argv - char * - list of arguments as strings
//
void argChecker(int argc, char* argv[]){
	
	// if one argument assume it is a port
	if(argc < 2){
		printf("Usage: <PORT> <client id>\n");
		exit(1);
	}

	if( strnlen(argv[1], 7) > 5){
		printf("Your port is too long... Exiting\n");
		exit(1);
	}

}


//                 setupPacket()
//
// This function sets up the packet struct and prepares it for sending
//
// @param: clientID - uint8 - id of the client
// @param: segmentNum - uint8 - packet number in the sequence
// @param: payload - uint8 * - data payload 
// @param: startOfPack - uint16 - start of packet code
// @param: packetType - uint16 - packet type (ACC_PERMISSION, NOT_PAID, NOT_EXIST, 
// ACCESS_OK)
// @param: endOfPack - uint16 - end of packet identifier
// @param: payloadType - bool - true if payload is char *, false if uint8 *
// @param: rejectCode - uint16 - reject code for reject packets
// @param: technology - uint8 - tech type (2g,3g,4g,5g)
// @param: phoneNumber - uint32 - phone number that user is connecing as
//
// @return: newPacket - packetInfo - Newly created packet based of prameters
//
packetInfo setupPacket(uint8_t clientID, uint8_t segmentNum, uint8_t *payload, 
uint16_t startOfPack, u_int16_t packetType, uint16_t endOfPack, bool payloadType, 
uint16_t rejectCode, uint8_t technology, uint32_t phoneNumber){

	packetInfo newPacket;

	newPacket.startOfPacketID = startOfPack;
	newPacket.endOfPacketID = endOfPack;
	newPacket.clientID = clientID;
	newPacket.packetTypeID = packetType;

	newPacket.segmentNum = segmentNum;
	newPacket.technology = technology;
	newPacket.phoneNumber = phoneNumber;

	int count =0;
	int countTech=0;
   	while (phoneNumber !=0) {   
    	++count;  
    	phoneNumber/=10;
   	} 
	while (technology !=0) {   
    	++countTech;  
    	technology/=10;
   	} 

	newPacket.payloadLen = count + countTech;
	
	return newPacket;
} 


//              packetPrinter()
//
// This function prints out the contents of a packet struct
//
// @param: packet - packetInfo - packet to be printed 
//
void packetPrinter(packetInfo packet){

	printf("[%#x, %d, ",packet.startOfPacketID, packet.clientID);
	printf("%#x, %d, ", packet.packetTypeID, packet.segmentNum);

	printf("%d, ",  packet.payloadLen);

	printf("[%" PRIu32 ", ", packet.technology);
	printf("%u], ",packet.phoneNumber);

	printf("%#x]\n", packet.endOfPacketID);



}

//              analyzePacket()
//
// This function will analyze a packet to determine if it is safe to use.
//
// @param: packet - packetInfo - packet to be analyzed
// @param: prevSegment - uint8 - segment number of the previous packet
//
// @return: packetID - uint16 - Packet type ID
//
uint16_t analyzePacket(packetInfo packet, uint8_t prevSegment, int dbSize){
	
	// Check Valid packet
	if(packet.endOfPacketID != END_OF_PACKET){
		return END_OF_PACKET_MISSING;
	}
	if(packet.segmentNum == prevSegment){
		return DUPLICATE_PACKET;
	}
	if((packet.segmentNum != prevSegment+1) && 
	(packet.segmentNum != prevSegment)){
		return OUT_OF_SEQUENCE;
	}
	

	// Count number of digets (lenght of payload)
	// Counts length of phonenumber and technology
	int count =0;
	int countTech=0;
	uint32_t phoneNumber = packet.phoneNumber;
	uint8_t technology = packet.technology;
   	while (phoneNumber !=0) {   
    	++count;  
    	phoneNumber/=10;
   	} 
	while (technology !=0) {   
    	++countTech;  
    	technology/=10;
   	} 

	uint8_t payloadLen = count + countTech;
	if(packet.payloadLen != payloadLen ){
		return LEN_MISMATCH;
	}

	
	
	switch (packet.packetTypeID){
	case ACC_PERMISSION:
		for(int i=0; i< dbSize; ++i){
			if(packet.phoneNumber == atoi(phoneNum[i])){
				// printf("Found the phone number!\n");
				if(packet.technology == atoi(tech[i])){
					// printf("Found Technology\n");
					if(atoi(paid[i]) == 1){
						printf("Access OK\n");
						return ACCESS_OK;
					}else{
						printf("NOT PAID\n");
						return NOT_PAID;
					}
				} else{
					printf("WRONG TECHNOLOGY\n");
				}
				
			}
		}
		return NOT_EXIST;
		break;
	
	default:
		return packet.packetTypeID;
		break;
	}
	
	printf("FOUND NO phone number in the DB");
	return NOT_EXIST;
	
}

//          procedureOneCLient()
//
// This function sets up client packets for part 1 of the test of my algorithm
//
// @param: clientID - uint8 - id of the client to add to packets
// @param: payload - uint8 ** - array of payloads to add to packets (in order)
// @param: packets - packet struct that is being filled
//
void procedureOneClient(uint8_t clientID, uint8_t payload [5][255], packetInfo *packets, 
int numOfPackets, uint32_t phoneNumber[4], uint8_t technology[]){
		
	// Set rejecCode to 0
	uint16_t rejectCode = 0;

	// Start segmentNumber at 0, so packet number starts at 1
	// using ++segmentNum increments first packet to 1 before
	// creating a packet
	uint8_t segmentNum = 0; 
	
	// Create packets
	for(int i=0;i<numOfPackets; ++i){

		packets[i] = 
		setupPacket(clientID++, ++segmentNum, payload[i], START_OF_PACKET, 
     	ACC_PERMISSION, END_OF_PACKET, true, rejectCode, technology[i], phoneNumber[i]);

	}
}


//                 dbReader()
//
// This function reads from a database of phone numbers stored in db.txt
//
// @return - dbSize - int - number of subscribers in the DB
//
int dbReader(char phoneNum[25][20], char tech[25][3], char paid[25][3]){
	int dbSize=0;
	FILE * fp;
    char line[20];
    size_t len = 0;
    // ssize_t len;
	printf("ENTERED FUCNTION\n");
    fp = fopen("serverBin/db.txt", "r");
    if (fp == NULL){
        exit(1);
	}
	printf("After fp\n");

	int x = -1;
	int i;
    // while ((len = getline(&line, &len, fp)) != -1) {
	while(fgets(line,20,fp)){
	
		++dbSize;
        // printf("Retrieved line of length %zu:\n", len);
        // // printf("%s", line);
		if((len = strlen(line)) == 0)
			break;
		for(i = 0; i< len; ++i){
			// printf("CHAR: [%c]\n", line[i]);
			if(line[i] == ' '){
				break;
			}
		}

		// the first i chars is the IP address
		strncpy(phoneNum[++x], line, i);
		phoneNum[x][i] = 0;

		// printf("PHOEN NUM: [%s]\n", phoneNum[x]);

		int end = 3;
		if(line[len-1] == '\n')
			end = 4;
		
		// copy everything after space (i+1) to the tech list
		strncpy(tech[x], &line[i+1],len-i-end);
		tech[x][len-i-1] = 0;

		// printf("TECH: [%s]\n", tech[x]);

		// copy everything after space (i+1) to the port list
		strncpy(paid[x], &line[i+4],len);
		paid[x][1] = 0;
		
		// printf("PAID : [%s]\n", paid[x]);
    }

    fclose(fp);

	return dbSize;
}



//           u8from32
//
// This functoin converts a uint32 number to an array of uint8
// The goal is to have the payload store a 32bit phone number
// as a uint8 payload array that includes the technology (3g,2g,4,5g)
// in index 0 of the array
//
// @param: b - uint8 [5] - uint8 array of size 5
// @param: u32 - uint32 - uint32 numbeer to convert
// @param: tech - uint8 - technology for the phone number (3g,2g,4,5g)
//
void u8from32 (uint8_t b[5], uint32_t u32, uint8_t tech){
    
	b[4] = (uint8_t)u32;
    b[3] = (uint8_t)(u32>>=8);
    b[2] = (uint8_t)(u32>>=8);
    b[1] = (uint8_t)(u32>>=8);
	b[0] = tech;
}