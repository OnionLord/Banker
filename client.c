/*------------------------------------------------------------------------------
CS-450 Fall 2013
Programming Assignment #2: Deadlock Avoidance using the Banker's Algorithm
Written By:
1- Ryan Robertson
2- Byron A. Craig IV
Submitted on: 11/03/2013
------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "resource.h"

/* Global Variables */
int available[STATIC_RESOURCE];
int allocated[STATIC_RESOURCE];
int rand_num;
int res_type;
int msg_status;
int serial_num;
int queID;
int timer;
int bank_id;
int bankerID = 0;
bank_info info;
msg_buf msg;
msg_buf msg2;
char line[] = "-------------------------------------\n";

/*------------------------------------------------------------------------------
client_process - Determines the random claim of resources the client will
need and sends a message to the banker stating what they need.
------------------------------------------------------------------------------*/
int
client_process (int id) {
	/* Local Variables */
	int purpose_id;
	int c = 1;
	int count = 0;
	int iter = 0;
	int i = 0;
	int dataIn = 0;
	int loop = 1;
	FILE *file;
	key_t msg_key = ACCT_NUM;
	msg.serial_num = id * 1000 + 1;
    
    /* Opening the file once again to grab data */
    file = fopen ("initial.data", "r");
	
	/* While loop storing the data from the file */
	while (fscanf(file, "%d", &dataIn) != EOF) {
		if (count == 0) {
		    res_type = dataIn;
		} // end if
		else if (count > 0) {
			available[i] = dataIn;
			i++;
		} // end else if
		count++;
	} // end while
	
	/* Printing which client is asking for resources */
	printf("This is Client #%d process (id = %d).\n", id, getpid());
    
    /* These statements are finding the banker's queue ID */
    /* This is for the purpose of sending messages to the banker */
	if ((bank_id = msgget(msg_key, IPC_CREAT | IPC_EXCL | 0660)) == -1) {
		bank_id = msgget(msg_key, IPC_CREAT | 0660);
		printf("Client found the Banker's mailbox 0x%X with qid = %d\n", 
		msg_key, bank_id);
	} // end if
	else {
		printf("Failed to find banker's mailbox");
		msgctl(bank_id, IPC_RMID, NULL);
		exit(-2);
	} // end else
    
    /* Creating the queue ID for the client */
	msg_key += id;
	queID = msgget(msg_key, IPC_CREAT | 0660);
    
    /* Checking to make sure the creation was a failure or success */
	if (queID < 0) {
		printf("Failed to create client mailbox 0x%X\n", msg_key);
		exit(-2);
	} // end if
	else {
		printf("Client process created mailbox 0x%X with qid = %d\n", 
		msg_key, queID);
	} // end else
	
	/* Printing # of resources from the data file */
	printf("Number of resource types = %d.\n\n", res_type);
    
    /* Randomly generating the claim and sending the claim to the banker */
    random_max_claim ();
    
    /* Send initial request message */
    purpose_id = 3;
    msg.clients += 1;
    printf("\n\n\n%d\n\n\n", msg.clients);
    msg.serial_num = (id * 1000) + 1;
    send_message (bank_id, id, queID, purpose_id);
    regmessage_sent(id);
    
    /* Receiving the messages from the banker */
    for(;;) {
		msg_status = msgrcv(queID, &msg2, MSG_INFO_SIZE, msg2.type, 0);
		msg.serial_num++; // increment serial number
		
		/* Imcrementing number of iterations for client */
		if (msg2.purpose < 9) {
			iter++;
		} // end if
		else if (msg2.purpose == 12) {
			iter++;
		} // end else if

		/* Checking to see if the client fails to receive */
		if (msg_status < 0) {
			printf("Failed to receive message from client on qid = %d\n"
			, queID);
			exit(-2);
		} // end if
		else {
			/* Switch/Case performing operations */
			switch (msg2.purpose) {
				case 4:
					/* Request Granted by the banker */
					printf("\nReceived the following msg at queueID %d :\n",
							queID);
					printf("#    %d-Granted,  senderID: %d resources: { "
							"%d %d %d %d %d }", c, bankerID,
							msg.request[0], msg.request[1], msg.request[2], 
							msg.request[3], msg.request[4]);
					printf("\n");
					
					/* for loop adding request to be allocated */
					for (i = 0; i < res_type; i++) {
						msg.allocated[i] = msg.request[i];
						msg.request[i] = msg.request[i] - msg.allocated[i];
					} // end for
					
					/* Outputs and message sending */
					allocated_output(id);
					sleepy();
					release_vector();
					purpose_id = 2;
					relmessage_sent(id);
					send_message (bank_id, id, queID, purpose_id);
					break;
				case 5:
					/* Request denied due to deadlock safety */
					break;
				case 6:
					/* Request denied by the banker */
					printf("\nReceived the following msg at queueID %d :\n",
							queID);
					printf("#    %d-Excessive,  senderID: %d resources: { "
							"%d %d %d %d %d }", c, bankerID,
							msg.request[0], msg.request[1], msg.request[2], 
							msg.request[3], msg.request[4]);
					printf("\n");
					allocated_output(id);
					sleepy();
					decrement_request();
					purpose_id = 1;
					reqmessage_sent(id);
					send_message (bank_id, id, queID, purpose_id);
					break;
				case 7:
					/* Resources released successfully */
					printf("\nReceived the following msg at queueID %d :\n",
							queID);
					printf("#    %d-Rel-Success,  senderID: %d resources: { "
							"%d %d %d %d %d }", c, bankerID,
							msg.release[0], msg.release[1], msg.release[2], 
							msg.release[3], msg.release[4]);
					printf("\n");
					allocated_output(id);
					sleepy();
					
					/* Checking number of iterations for client */
					if (iter == 10) {
						msg2.purpose = 11;
						break;
					} // end if
					
					release_vector();
					purpose_id = 2;
					relmessage_sent(id);
					send_message (bank_id, id, queID, purpose_id);
					break;
				case 8:
					/* Resource release failed due to excess release */
					printf("\nReceived the following msg at queueID %d :\n",
							queID);
					printf("#    %d-Rel-Failed,  senderID: %d resources: { "
							"%d %d %d %d %d }", c, bankerID,
							msg.release[0], msg.release[1], msg.release[2], 
							msg.release[3], msg.release[4]);
					printf("\n");
					allocated_output(id);
					printf("Client %d: Resource Release Failed\n", id);
					sleepy();
					
					/* Checking number of iterations for client */
					if (iter == 10) {
						msg2.purpose = 11;
						break;
					} // end if
					
					release_vector();
					purpose_id = 2;
					relmessage_sent(id);
					send_message (bank_id, id, queID, purpose_id);
					break;
				case 9:
					/* Initial max claims accepted */
					for (i = 0; i < res_type; i++) {
						msg.need[i] = msg.res[i];
					}
					printf("\nReceived the following msg at queueID %d :\n",
							queID);
					printf("#    %d-Reg-Success,  senderID: %d resources: { "
							"%d %d %d %d %d }", c, bankerID,
							msg.res[0], msg.res[1], msg.res[2], msg.res[3], 
							msg.res[4]);
					printf("\n");
					allocated_output(id);
					sleepy();
					request_vector();
					purpose_id = 1;
					reqmessage_sent(id);
					send_message (bank_id, id, queID, purpose_id);
					break;
				case 10:
					/* Initial max claims denied */
					printf("\nReceived the following msg at queueID %d :\n",
							queID);
					printf("#    %d-Reg-Failed,  senderID: %d resources: { "
							"%d %d %d %d %d }", c, bankerID,
							msg.res[0], msg.res[1], msg.res[2], msg.res[3], 
							msg.res[4]);
					printf("\n");
					allocated_output(id);
					decrement_claim();
					purpose_id = 3;
					regmessage_sent(id);
					send_message (bank_id, id, queID, purpose_id);
					break;
				case 12:
					/* Request denied -- resources unavailable */
					break;
			} // end switch/case
		} // end else
		
		/* Ending the program */
		if(msg2.purpose == 11) {
			printf("\nClient %d is shutting down...", id);
			printf("Press Enter to continue");
			getchar();
			relall_sent(id);
			printf("\nGoodbye!\n");
			purpose_id = 11;
			send_message (bank_id, id, queID, purpose_id);
			break;
		} // end if
		
		c++;
	} //end for
		
	msgctl(queID, IPC_RMID, NULL); // Wiping the mailbox ID
    
	return 0;
} // end client_process

/*------------------------------------------------------------------------------
allocated_output - This method is what prints out what requests have been now
allocated for each client.
------------------------------------------------------------------------------*/
int
allocated_output (int cid) {
	/* Local Variables */
	int i;
	
	/* Updating allocated array after rel-success */
	if (msg2.purpose == 7) {
		for (i = 0; i < res_type; i++) {
			msg.allocated[i] = msg.allocated[i] - msg.release[i];
		} // end for
	} // end if
	
	/* For loop to transfer data to local array */
	for (i = 0; i < res_type; i++) {
		allocated[i] = msg.allocated[i];
	} // end for
	
	/* Print statements */
	printf("%s", line);
	printf("Allocated[client %d]  = ( %d %d %d %d %d )", cid, allocated[0]
			, allocated[1], allocated[2], allocated[3], allocated[4]);
	printf("\n%s", line);
	
	return 0;
} // end allocated_output

/*------------------------------------------------------------------------------
decrement_claim - Decrements each integer within the array
if and only if the banker sends back a code stating that
their register was denied.
------------------------------------------------------------------------------*/
int
decrement_claim () {
	/* Local Variables */
	int i;
	
	/* Decrementing each integer */
	for (i = 0; i < res_type; i++) {
		if (msg.res[i] != 0) {
		    msg.res[i] = msg.res[i] - 1;
		} // end if
	} // end for
	
	return 0;
} // end decrement_claim

/*------------------------------------------------------------------------------
decrement_request - Decrements each integer within the 
array if and only if the banker sends back a code stating 
that their request was denied.
------------------------------------------------------------------------------*/
int
decrement_request () {
	/* Local Variables */
	int i;
	
	/* Decrementing the request array */
	for (i = 0; i < res_type; i++) {
		if (msg.request[i] != 0) {
			msg.request[i] = msg.request[i] - 1;
		} // end if
	} // end for

	return 0;
} // end decrement_request

/*------------------------------------------------------------------------------
random_max_claim - This method generates an array of random
numbers that are to act as the claim of resources for the
client to request to the banker.
------------------------------------------------------------------------------*/
int
random_max_claim () {
	/* Local Variables */
	int ii;
	int range;
	srand(time(NULL));
	range = (10 - 1) + 1; // Setting range
	
	/* For loop randomly generating each number in array */
	for (ii = 0; ii < 5; ii++) {
		msg.res[ii] = rand() % range + 1;
	} // end for
	
	return 0;
} // end random_max_claim

/*------------------------------------------------------------------------------
relall_sent - This method is called after the ten iterations have been completed
for the client. Once these are done, all resources will be released and then the
program will clear and then exit.
------------------------------------------------------------------------------*/
int
relall_sent (int cid) {
	/* Printing out the message sending format */
	printf("\nSent the following registration msg to queueID %d :\n", bank_id);
    printf("#%d-Release-ALL,\t senderID: %d,  resources: { 0 0 0 0 0 }"
			, msg.serial_num, cid);
	printf("\n");
	printf("%s", line);
	printf("Allocated[client %d]  = ( 0 0 0 0 0 )", cid);
	printf("\n%s", line);
} // end relall_sent

/*------------------------------------------------------------------------------
regmessage_sent - This is when the client has received an answer from the banker
and then sends a reply back with either an updated request, or register, or even
the release of resources.
------------------------------------------------------------------------------*/
int
regmessage_sent (int cid) {
	/* Printing out the message sending format */
	printf("\nSent the following registration msg to queueID %d :\n", bank_id);
    printf("#%d-Register,\t senderID: %d,  resources: { %d %d %d %d %d }"
			, msg.serial_num, cid, msg.res[0], msg.res[1], msg.res[2],
			msg.res[3], msg.res[4]);
	printf("\n");
} // end regmessage_sent

/*------------------------------------------------------------------------------
relmessage_sent - This is when the initial max claim from the client has been
accepted by the banker as well as the requested resources that the client
wanted to be allocated. After all of this has been accepted by the banker, this
output is printed which informs that a request for release has been sent.
------------------------------------------------------------------------------*/
int
relmessage_sent (int cid) {
	/* Printing out the message sending format */
	printf("\nSent the following registration msg to queueID %d :\n", bank_id);
    printf("#%d-Release,\t senderID: %d,  resources: { %d %d %d %d %d }"
			, msg.serial_num, cid, msg.release[0], msg.release[1], 
			msg.release[2], msg.release[3], msg.release[4]);
	printf("\n");
} // end relmessage_sent

/*------------------------------------------------------------------------------
reqmessage_sent - This is when the initial max claim from the client has been
accepted by the banker. It prints out the following formatted statement of the
requested resources that are wanting to be allocated.
------------------------------------------------------------------------------*/
int
reqmessage_sent (int cid) {
	/* Printing out the message sending format */
	printf("\nSent the following registration msg to queueID %d :\n", bank_id);
    printf("#%d-Request,\t senderID: %d,  resources: { %d %d %d %d %d }"
			, msg.serial_num, cid, msg.request[0], msg.request[1], 
			msg.request[2], msg.request[3], msg.request[4]);
	printf("\n");
} // end reqmessage_sent

/*------------------------------------------------------------------------------
release_vector - This method occurs after the registration has been accepted by
the banker. Now, a randomly generated request will be created that the client
will use to request resources within its initial claim.
------------------------------------------------------------------------------*/
int
release_vector () {
	/* Local Variables */
	int random = rand() % 10; // For testing rel-failed
	int range_1,range_2,range_3,range_4,range_5;
	
	/* Setting the ranges of each array item */
	range_1 = allocated[0];
	range_2 = allocated[1];
	range_3 = allocated[2];
	range_4 = allocated[3];
	range_5 = allocated[4];
	
	/* Randomly generated requests */
	if (range_1 == 0) {
		msg.release[0] = 0;
	} // end if
	else {
		msg.release[0] = (1 + rand() % range_1);
	} // end else
	
	if (range_2 == 0) {
		msg.release[1] = 0;
	} // end if
	else {
		/* For Testing the Rel_failed */
		if (random <= 4) {
			msg.release[1] = (1 + rand() % range_2);
		} // end if
		else {
			msg.release[1] = (1 + rand() % range_2) + res_type;
		} // end else
	} // end else
	
	if (range_3 == 0) {
		msg.release[2] = 0;
	} // end if
	else {
		msg.release[2] = (1 + rand() % range_3);
	} // end else
	
	if (range_4 == 0) {
		msg.release[3] = 0;
	} // end if
	else {
		msg.release[3] = (1 + rand() % range_4);
	} // end else
	
	if (range_5 == 0) {
		msg.release[4] = 0;
	} // end if
	else {
		msg.release[4] = (1 + rand() % range_5);
	} // end else
	
	return 0;
} // end reqlease_vector

/*------------------------------------------------------------------------------
request_vector - This method occurs after the registration has been accepted by
the banker. Now, a randomly generated request will be created that the client
will use to request resources within its initial claim.
------------------------------------------------------------------------------*/
int
request_vector () {
	/* Local Variables */
	int i;
	int range_1,range_2,range_3,range_4,range_5;
	
	/* Setting the ranges of each array item */
	range_1 = msg.need[0];
	range_2 = msg.need[1];
	range_3 = msg.need[2];
	range_4 = msg.need[3];
	range_5 = msg.need[4];
	
	/* Randomly generated requests */
	if (range_1 == 0) {
		msg.request[0] = 0;
	} // end if
	else {
		msg.request[0] = (1 + rand() % range_1) + res_type;
	} // end else
	
	if (range_2 == 0) {
		msg.request[1] = 0;
	} // end if
	else {
		msg.request[1] = (1 + rand() % range_2) + res_type;
	} // end else
	
	if (range_3 == 0) {
		msg.request[2] = 0;
	} // end if
	else {
		msg.request[2] = (1 + rand() % range_3) + res_type;
	} // end else
	
	if (range_4 == 0) {
		msg.request[3] = 0;
	} // end if
	else {
		msg.request[3] = (1 + rand() % range_4) + res_type;
	} // end else
	
	if (range_5 == 0) {
		msg.request[4] = 0;
	} // end if
	else {
		msg.request[4] = (1 + rand() % range_5) + res_type;
	} // end else
	
	return 0;
} // end request_vector

/*------------------------------------------------------------------------------
send_message - Sets up all of the details to be sent 
in the response messages to the banker's mailbox.
------------------------------------------------------------------------------*/
int
send_message(int bank_id, int c_id, int queID, int purp_id){
	/* Local Variables */
    int msgStatus;
    
    /* Setting the values */
    msg.purpose = purp_id;
    msg.type = 1;
    msg.clientID = c_id;
    msg.qid = queID;
    
    /* Sending the message to the banker */
    msgStatus = msgsnd(bank_id, &msg, MSG_INFO_SIZE, 0);
    
    if (msgStatus < 0) {
        printf("Failed to receive message from client on qid = %d\n"
        , queID);
        exit(-2);
    } // end if
    
    return 0;
} // end send_message

/*------------------------------------------------------------------------------
sleep - Once the banker has successfully added the register, or any kind of
decision the banker makes during the request and release, the program will
then sleep for a random number of milliseconds. Once this is over, the program
will start back up again and continue its process.
------------------------------------------------------------------------------*/
int
sleepy () {
	/* Setting a random amount of ms to sleep */
	timer = (100 + rand() % 2000);
	printf("\nGoing to sleep for %d millseconds . . . .",
			timer);
	usleep(timer);
	printf(" I am back!\n");
	return 0;
} // end sleep
	
/*------------------------------------------------------------------------------
main - Runs the complete client program.
------------------------------------------------------------------------------*/
int
main (int argc, char * argv[]) {
	/* Local Variables */
	int c_id;
	
	/* Setting the command line arg to an int */
	/* Using that arg to determine which client */
	c_id = atoi(argv[1]);
	client_process (c_id);
    
	return 0;
} // end main
