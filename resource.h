/*------------------------------------------------------------------------------
CS-450 Fall 2013
Programming Assignment #2: Deadlock Avoidance using the Banker's Algorithm
Written By:
1- Ryan Robertson
2- Byron A. Craig IV
Submitted on: 11/03/2013
------------------------------------------------------------------------------*/

#include <errno.h>
#define MAX_CLIENTS 10
#define MAX_ATTEMPTS 10
#define STATIC_RESOURCE 5

/* struct to set up queue for message sending */
typedef struct {
	long type;
	int qid;
	int clients;
	int serial_num;
    int clientID;
    int release[STATIC_RESOURCE];
    int request[STATIC_RESOURCE];
	int res[STATIC_RESOURCE];
	int need[STATIC_RESOURCE];
	int allocated[STATIC_RESOURCE];
	int purpose;
} msg_buf ;

/* struct to set up the banker */
typedef struct{
	int counter;
	int res_type;
    int available[STATIC_RESOURCE];
} bank_info ;

#define MSG_INFO_SIZE (sizeof(msg_buf) - sizeof(long))
#define ACCT_NUM 0x5400000

