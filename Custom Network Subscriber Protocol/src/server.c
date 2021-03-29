//           server.c
//
// This file runs the server side of the connection. This server simply replies
// to packets fromt the client after verifying its contents.
//
// @author: Ramzey Ghanaim

#include "udp_helper.c"

int main(int argc, char* argv[]){
	
	// Check for valid arguments
	argChecker(argc, argv);
	// char array[25][20];

	// char phoneNum[25][20]; char tech[25][3]; char paid[25][3];

	int dbSize = dbReader(phoneNum, tech, paid);

	for(int i=0; i< 3; ++i){
		printf("Phone Number: [%s]\n", phoneNum[i]);
		printf("Technology: [%s]\n", tech[i]);
		printf("Paid Status: [%s]\n", paid[i]);
		printf("-------------------------------\n");
	}

	// Create a network connection
	connectInfo myConnection = makeConn(atoi(argv[1]), "server");
	
	// Declare rx and tx packets
	packetInfo rxPacket;
	packetInfo txPacket;

	// Create client size var
	socklen_t csize= sizeof(myConnection.client);
	uint16_t prevSegment = 0;

	// Continously wait for clients...
	while(1){
		
		// Reset rx buffer
		memset(rxPacket.payload,0,LEN_OF_PAYLOAD);
		
		printf("Waiting for clients...\n");
		if(recvfrom(myConnection.socketFD,&rxPacket, sizeof(rxPacket),0,
					(struct sockaddr *) &(myConnection.client), &csize) < 0){
				perror("Not received properly");
		}
		else{
			// Reset tx payload
			memset(txPacket.payload,0,LEN_OF_PAYLOAD);
			
			// Print out the just received packet, and analyze it
			packetPrinter(rxPacket);
			uint16_t packetCode = analyzePacket(rxPacket, prevSegment, dbSize);
			
			// Asssume REJECT return packet, until prove otherwise
			txPacket = setupPacket(rxPacket.clientID,rxPacket.segmentNum,
				0, START_OF_PACKET, REJECT, END_OF_PACKET, false,
				packetCode, 2, 0 );
			
			// Prove otherwise
			if(packetCode == DATA){

				// Got packet corretcly, prevSegment = current one
				prevSegment = rxPacket.segmentNum;

				// Make return packet
				txPacket.packetTypeID = ACK;
				printf("Found No issues, just ACK on packet: %i\n",
				rxPacket.segmentNum);
				
			}else if (packetCode == END_OF_PACKET_MISSING){
				
				printf("Server: Missing END_OF_PACKET_ID on packet: %d\n ",
				rxPacket.segmentNum);

				// Always increment expected segment num
				prevSegment+=1;

			}else if (packetCode == OUT_OF_SEQUENCE){
				printf("Server: Packet arrived out of sequence. ");
				printf("Expected: %d, Received: %d\n", prevSegment+1,
				rxPacket.segmentNum);
			}else if(packetCode == LEN_MISMATCH){
				printf("Server: payload length mismatch. ");
				printf("Expected: %lu, Received: %d\n",
				strnlen( (char *) rxPacket.payload, LEN_OF_PAYLOAD),
				rxPacket.payloadLen);
				
				// Always increment expected segment num
				prevSegment+=1;
			
			}else if(packetCode == DUPLICATE_PACKET){
				printf("Server: Received Duplicate packet.");
				printf("Duplicate Packet Segment Number: %d",
				rxPacket.segmentNum);
			}else if(packetCode == ACCESS_OK){
				printf("Server: User Access OK.\n");
				prevSegment+=1;
				txPacket.packetTypeID = ACCESS_OK;
				txPacket.technology = rxPacket.technology;
				txPacket.payloadLen = rxPacket.payloadLen;
				txPacket.phoneNumber = rxPacket.phoneNumber;

			}else if(packetCode == NOT_PAID){
				printf("Server: User Not Paid\n");
				prevSegment+=1;
				txPacket.packetTypeID = NOT_PAID;
				txPacket.technology = rxPacket.technology;
				txPacket.payloadLen = rxPacket.payloadLen;
				txPacket.phoneNumber = rxPacket.phoneNumber;

			} else if(packetCode == NOT_EXIST){
				printf("Server: User does not exist\n");
				txPacket.packetTypeID = NOT_EXIST;
				txPacket.technology = rxPacket.technology;
				txPacket.payloadLen = rxPacket.payloadLen;
				txPacket.phoneNumber = rxPacket.phoneNumber;
			}

			// Send ACK or error Packet back to client
			printf("About to send:\n");
			packetPrinter(txPacket);
			if(sendto(myConnection.socketFD, &txPacket, 
            sizeof(txPacket), 0, (struct sockaddr *) &(myConnection.client),
			csize) < 0 ){
                perror("Failed to send");
            }

			printf("------------------------------------------- ");
		}
	}

	// Close when finished.
	close(myConnection.socketFD);
	return 0;
}
