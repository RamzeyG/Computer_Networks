//            client.c
//
// This file runs the client side of the connection. This program will create
// 4 packets to send over to the server, send them over, and print out
// the server's response. The goal is to mimic cell phone subscribers asking
// for permission to join the network based off a database stored on the server
//
// @author: Ramzey Ghanaim

#include "udp_helper.c"


int main(int argc, char* argv[]){
	
    // Check for valid arguments
    argChecker(argc, argv);

    // Create a conneciton as the client
	connectInfo myConnection = makeConn(atoi(argv[1]), "client");
    
    // Establish client ID
    uint8_t clientID;
    // uint8_t segmentNum = 0;

    if (argc > 2){
        if (atoi(argv[2]) > 255){
            printf("ERROR. Client ID is too big\n");
            exit(1);
        }
        clientID = atoi(argv[2]);
        printf("Client ID is: %d", clientID);
    }else{
        clientID = 66;
    }
    
   // Crete the tx buffer and payload
    int DEV_TEST_SIZE = 4;
    uint8_t payload[5][255];
    uint8_t technology[DEV_TEST_SIZE];
    uint32_t phoneNumber[DEV_TEST_SIZE];


    technology[0] = 4;
    technology[1] = 3;
    technology[2] = 2;
    technology[3] = 4;

    phoneNumber[0] = 4040984572;
    phoneNumber[1] = 2049858752;
    phoneNumber[2] = 1369889057;
    phoneNumber[3] = 3567479395;

   

    // Set up payload    
    for (int i=0; i< 5; ++i){
        memset(payload[i], 0, LEN_OF_PAYLOAD);

    }
    u8from32(payload[0],4040984572, 4);
    u8from32(payload[1],2049858752, 3);
    u8from32(payload[2],1369889057, 2);
    u8from32(payload[3],3567479395, 4);

    // Done setting up Payload


   
    // DEV_TEST_SIZE Packet Initalizers
    packetInfo myPackets[DEV_TEST_SIZE];
    procedureOneClient(clientID, payload, myPackets, DEV_TEST_SIZE, 
    phoneNumber, technology);

    packetPrinter(myPackets[0]);

    // create client size var
	socklen_t ssize= sizeof(myConnection.server);
    
    // Set a time out value once client connects
	setsockopt(myConnection.socketFD, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    // Initalize up Packet sender counters, timers, 
	int ackTimer = 0;
    int retryCount = 0;
    bool send = true;

    // Declare Packet sender variables
    packetInfo rxPacket;
    uint16_t packetCode;

    // Send packet DEV_TEST_SIZE times with this for loop
    for(int i=0; i< DEV_TEST_SIZE; ++i){
        
        if(send){
            // Send packet
            printf("-------------------------------------------------------\n");
            printf("About to send: ");
            packetPrinter(myPackets[i]);
            if(sendto(myConnection.socketFD, (void *) &myPackets[i], 
            sizeof(myPackets[i]), 0, (struct sockaddr *) &(myConnection.server),
            ssize) < 0 ){
                perror("Failed to send");
            }
        }

        //Prep for recv
        memset(rxPacket.payload, 0, LEN_OF_PAYLOAD);

        // Recv packet
        if(recvfrom(myConnection.socketFD,&rxPacket, sizeof(rxPacket),0,
        (struct sockaddr *) &(myConnection.client), &ssize) < 0){
            perror("Timeout");

            // Try waiting for ack packet again, without sending
            if(++ackTimer <= 3){
                printf("ackTimmer: %i\n", ackTimer );
                
                // Don't send
                send = false;       
            }
            
            // if ackTimmer expired... retransmit
            else if(++retryCount <= 3){
                printf("Retransmitting...%i\n", retryCount);
                
                // Allow sending
                send = true;
                // reset ackTimer
                ackTimer = 0;

            }
            
            // No ack was received from server after sending packet 3 times
            else{
                printf("Server does not respond\n");
                
                // reset for next packet
                ackTimer = 0;
                send = true;
                retryCount = 0;

                // Go to next itteration of for loop
                continue;
            }

            --i;
        }else{
            // If we are here, we got a packet from the server within timeout
            // interval.

            // Reset Variables
            ackTimer = 0;
            send = true;
            retryCount = 0;

            // Grab the packet type
            packetCode = analyzePacket(rxPacket, rxPacket.segmentNum-1, 3);
            
            printf("Received Packet: ");
            packetPrinter(rxPacket);

            // Decide what to do based of Packet Type
            switch (packetCode)
            {
            case ACK:
                printf("Client: Got ACK Succesfully.\n");
                break;
            case END_OF_PACKET_MISSING:
                printf("Client: Got END_OF_PACKET_ID error.\n ");
                break;
            case OUT_OF_SEQUENCE:
                printf("Client: Got OUT_OF_SEQUENCE error...\n");
                break;
            case LEN_MISMATCH:
                printf("Client: Payload Length mismatch error\n");
                break;
            case DUPLICATE_PACKET:
                printf("Client: Received Duplicate packet error.\n");
                break;
            case DATA:
                printf("Client: Got Data packet\n");
                break;
            case ACCESS_OK:
                printf("Client: Access to the Network OK.\n");
                break;
            case NOT_PAID:
                printf("Client: Error. Subscriber has not paid\n");
                break;
            case NOT_EXIST:
                printf("Client: Error. Client does not exist in Data Base\n");
                break;
            case REJECT:
                printf("Client: Error. Client Rejected\n");
                break;
            default:
                printf("ERROR. Could not determine Packet type\n");
                break;
            }           
            
        }
    }


    // Close connection when complete.
    close(myConnection.socketFD);
    return 0;
}