//           primitives.h
//
// This file defines all primitive and statc values. Also, all required
// libraries are included from this file.
//
// @author: Ramzey Ghanaim

// Primitives
#define START_OF_PACKET 0xFFFF
#define END_OF_PACKET 0xFFFF
#define CLIENT_ID 0xFF
#define LEN_OF_PAYLOAD 0xFF

// Packet Types
#define DATA 0xFFF1
#define ACK 0xFFF2
#define REJECT 0xFFF3

// Reject Sub Codes
#define OUT_OF_SEQUENCE 0xFFF4
#define LEN_MISMATCH 0xFFF5
#define END_OF_PACKET_MISSING 0xFFF6
#define DUPLICATE_PACKET 0xFFF7


// Socket libraries
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>


// Static Variables
#define MAX_PACKET_SIZE 264//128000
#define TIMEOUT 4

// Packet Type DATA properties
#define START_OF_PACKET_INDEX 0
#define PAYLOAD_START_INDEX 7
#define CLIENT_ID_INDEX 2
#define PACKET_TYPE_INDEX 3
#define SEG_NUM_INDEX_DATA 5
#define PAYLOAD_LEN_INDEX 6



// pa2 Packet types
#define ACC_PERMISSION 0xFFF8
#define NOT_PAID 0xFFF9
#define NOT_EXIST 0xFFFA
#define ACCESS_OK 0xFFFB

// Subscriber Number
#define SUBSCRIBE_MAX_NUM 0xFFFFFFFF

// Technologies
uint8_t twoG = 2;
uint8_t threeG = 3;
uint8_t fourG = 4;
uint8_t fiveG = 5;


