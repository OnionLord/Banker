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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "resource.h"

/* Global Variables */
msg_buf msg, msg2;
bank_info info;
int bankerID = 0;
int msg_status;
int clients = 0;
int c = 0;
int i = 0;
int check;
int compare;
int rel_check;
int alloc[MAX_CLIENTS][STATIC_RESOURCE];
int needs[MAX_CLIENTS][STATIC_RESOURCE];
char line[] = "----------------------------------------\n";

/*------------------------------------------------------------------------------
read_file - This method reads the file and collects its
data to use for the available resources allowed for the
clients to allocate.
------------------------------------------------------------------------------*/
int
read_file () {
	/* Local Variables */
	FILE *file;
	int i = 0;
	int dataIn = 0;
	int count = 0;
	info.counter = 1;
	
	/* Opening the data file */
	file = fopen ("initial.data", "r");
	
	/* While loop to grab the data from the file
	   and store it in the array */
	while (fscanf(file, "%d", &dataIn) != EOF) {
		if (count == 0) {
		    info.res_type = dataIn;
		} // end if
		else if (count > 0) {
			info.available[i] = dataIn;
			i++;
		} // end else if
		count++; // increment the count
	} // end while
	return 0;
} // end read_file

/*------------------------------------------------------------------------------
banker_process - This method is used as the interface helper to show
what is going on in the program. It receives the message from client
and determines whether the client is asking for too much resources or
if the banker has the available resources.
------------------------------------------------------------------------------*/
int
banker_process () {
	/* Local Variables */
	int x = 0;
	int y = 0;
	int c = 0;
	int qid;
	key_t msg_key = ACCT_NUM;
    
    /* First printouts to open the banker */
	printf("This is the Banker process (id = %d).\n", getpid());
	printf("Number of resource types = %d.\n", info.res_type);
	
	/* Getting the queue ID to create the banker's box */
	qid = msgget( msg_key, IPC_CREAT | 0666 );
    
    /* If statement to check if the mailbox has been already created */
	if (qid < 0) {
		printf("Failed to create mailbox 0x%X\n", msg_key);
		exit(-2);
	} // end if
	else {
	    printf("Banker process created mailbox 0x%X with qid = %d\n\n", 
	    msg_key, qid);
	} // end else
	
	/* Printing out the initial allocation resources */
	printf("Initial allocation state is:\n");
	printf("%s", line);
	printf("\tAvailable[]    = ( ");
	for(i = 0; i < info.res_type; i++) {
		printf("%2d", info.available[i]);
	}
	printf("  )\n");
	printf("%s\n", line);
    
    /* Infinite loop to keep receiving messages from clients */
    for (;;) {
		/* receiving */
		msg_status = msgrcv(qid, &msg, MSG_INFO_SIZE, msg.type, 0);
		
		/* Incrementing everytime a new client comes in */
		if (msg.serial_num - (msg.clientID * 1000) == 1) {
			clients += msg.clients;
			printf("\n\n\n%d\n\n\n", clients);
		} // end if
		
		/* Exiting if all clients are gone */
		if (msg.clients == 0) {
			printf("\nBanker is shutting down...Press Enter to continue");
			getchar();
			printf("\nGoodbye!\n");
			break;
		} // end if
		
        /* Checking to see if the banker fails to receive */
        if (msg_status < 0) {
            printf("Failed to receive message from client on qid = %d\n"
            , qid);
            exit(-2);
        } // end if
        else {
			/* These check what outputs to print depending on the code */
			if (msg.purpose == 3) {
				/* Printing out the received message and data from client */
	            printf("\nReceived following message at queueID %d\n", qid);
				printf("#%d-Register,\t senderID: %d,\t", msg.serial_num, 
						msg.clientID);
	            printf("resources: { %2d %2d %2d %2d %2d  }\n", msg.res[0],
						msg.res[1], msg.res[2], msg.res[3], msg.res[4]);
	            printf("Sent following registration response msg to queueID"
						" %d :\n", msg.qid);
			} // end if
			else if (msg.purpose == 1) {
				/* Printing out the received message and data from client */
	            printf("\n\nReceived following message at queueID %d\n", qid);
				printf("#%d-Request,\t senderID: %d,\t", msg.serial_num, 
						msg.clientID);
	            printf("resources: { %2d %2d %2d %2d %2d  }\n", msg.request[0],
						msg.request[1], msg.request[2], msg.request[3], 
						msg.request[4]);
	            printf("Sent following registration response msg to queueID"
						" %d :\n", msg.qid);
			} // end else-if
			else if (msg.purpose == 11) {
				printf("\n\nReceived following message at queueID %d\n", qid);
				printf("#%d-Release-ALL,  senderID: %d,  ", msg.serial_num, 
						msg.clientID);
	            printf("resources: { 0  0  0  0  0 }\n");
				
				/* Adding to the available */
	            for (i = 0; i < info.res_type; i++) {
					info.available[i] += msg.need[i];
				} // end for
				
				/* Clearing all data from all arrays */
				for (i = 0; i < info.res_type; i++) {
					msg.release[i] = 0;
				    msg.request[i] = 0;
					msg.res[i] = 0;
					msg.need[i] = 0;
					msg.allocated[i] = 0;
				} // end for
				
				/* Final output */
				success_output();
				clients -= 1; // Decrementing the # of clients
				
	        } // end else if
			else {
				/* Printing out the received message and data from client */
	            printf("\n\nReceived following message at queueID %d\n", qid);
				printf("#%d-Release,\t senderID: %d,\t", msg.serial_num, 
						msg.clientID);
	            printf("resources: { %2d %2d %2d %2d %2d  }\n", msg.release[0],
						msg.release[1], msg.release[2], msg.release[3], 
						msg.release[4]);
	            printf("Sent following registration response msg to queueID"
						" %d :\n", msg.qid);
			} // end else
			
			/* Switch/Case for sending messages back and forth */
            switch (msg.purpose) {
				case 1:
					/* Client is Requesting Resources */
					compare_request();
					
					/* Checking what purpose code to send */
					if (compare == 1) {
						msg2.purpose = 6;
						msg2.type = 2;
					} // end if
					else {
						msg2.purpose = 4;
						msg2.type = 2;
					} // end else
					request_output();
					
					/* Setting both instances' arrays equal */
					for (i = 0; i < info.res_type; i++) {
						msg2.request[i] = msg.request[i];
					} // end for
					
					/* Sending to the client */
					msgsnd(msg.qid, &msg2, MSG_INFO_SIZE, 0);
					break;
				case 2:
					/* Client is Releasing Resources */
					compare_release();
					
					/* Checking what purpose code to send */
					if (rel_check == 1) {
						msg2.purpose = 8;
						msg2.type = 2;
					} // end if
					else {
						msg2.purpose = 7;
						msg2.type = 2;
					} // end else
					release_output();
					
					/* Setting both instances' arrays equal */
					for (i = 0; i < info.res_type; i++) {
						msg2.release[i] = msg.release[i];
					} // end for
					
					/* Sending to the client */
					msgsnd(msg.qid, &msg2, MSG_INFO_SIZE, 0);
					break;
				case 3:
					/* Registering the claim with banker */
					compare_register();
					intial_output();
					
					/* Checking what purpose code to send */
					if (check == 1) {
					    msg2.purpose = 10;
					    msg2.type = 2;
					} // end if
					else {
						msg2.purpose = 9;
						msg2.type = 2;
					} // end else
					
					/* Setting both instances' arrays equal */
					for (i = 0; i < info.res_type; i++) {
						msg2.res[i] = msg.res[i];
					} // end for
					
					/* Sending to the client */
					msgsnd(msg.qid, &msg2, MSG_INFO_SIZE, 0);
					break;
				case 11:
					/* Does nothing */
					break;
			} // end switch/case
        } // end else
        
        /* Exiting once all clients have finished */
        if (clients == 0) {
			printf("\n\nBanker is shutting down...Press Enter to continue");
			getchar();
			printf("Goodbye!\n");
			break;
		} // end if
        
        info.counter++;
        c++;
    } // end for
    msgctl(qid, IPC_RMID, NULL); // Wiping queue ID
	return 0;
} // end banker_process

/*------------------------------------------------------------------------------
compare_register - Compares the array of available
resources that the banker contains and the array
of what the client is claiming they need.
------------------------------------------------------------------------------*/
int
compare_register () {
	/* Local Variables */
	int i;
	
	/* For loop for checking the arrays */
	for (i = 0; i < 5; i++) {
		if (msg.res[i] > info.available[i]) {
			check = 1;
			break;
		} // end if
		else {
			check = 0;
		} // end else
	} // end loop
	
	return 0;
} // end compare_register

/*------------------------------------------------------------------------------
compare_release - Compares the array of requested resources that the banker
has allocated and the array of what the client is now wanting to release.
------------------------------------------------------------------------------*/
int
compare_release () {
	/* Local Variables */
	int i;
	
	/* For loop for checking the register and request */
	for ( i = 0; i < info.res_type; i++) {
		if (msg.release[i] > msg.allocated[i]) {
			rel_check = 1;
			break;
		} // end if
		else {
			rel_check = 0;
		} // end else
	} // end for
	
	return 0;
} // end compare_release

/*------------------------------------------------------------------------------
compare_request - Compares the array of registered resources that the banker
contains and the array of what the client is now requesting.
------------------------------------------------------------------------------*/
int
compare_request () {
	/* Local Variables */
	int i;
	
	/* For loop for checking the register and request */
	for ( i = 0; i < info.res_type; i++) {
		if (msg.request[i] > msg.res[i]) {
			compare = 1;
			break;
		} // end if
		else {
			compare = 0;
		} // end else
	} // end for
	
	return 0;
} // end compare_request

/*------------------------------------------------------------------------------
intial_output - This outputs what happens when the request fails or succeeds
with the banker's availability.
------------------------------------------------------------------------------*/
int
intial_output () {
	/* Now checking to see if the client's claim was too high */
	/* If it was too high, an appropriate message is outputted */
	if (check == 1) {
		printf("#    %d-Reg-Failed, senderID: %d, ", info.counter,
				bankerID);
		printf("resources: { %2d %2d %2d %2d %2d  }\n", 
				msg.res[0], msg.res[1], msg.res[2], msg.res[3],
				msg.res[4]);
		printf("Initial allocation state is:\n");
		printf("%s", line);
		printf("\tAvailable[]    = ( ");
		for(i = 0; i < info.res_type; i++) {
			printf("%2d", info.available[i]);
		}
		printf("  )\n");
		printf("%s\n", line);
	} // end if
	else {
		printf("#    %d-Reg-Success, senderID: %d, ", info.counter,
				bankerID);
		printf("resources: { %2d %2d %2d %2d %2d  }\n",
				msg.res[0], msg.res[1], msg.res[2], msg.res[3],
				msg.res[4]);
		printf("%s", line);
		printf("\tAvailable[]    = ( ");
		for(i = 0; i < info.res_type; i++) {
			printf("%2d", info.available[i]);
		}
		printf("  )\n");
		success_output();
		printf("%s", line);
	} // end else
	return 0;
} // end intial_output

/*------------------------------------------------------------------------------
release_output - This method is for printing out whether the requested resources
that the client wants to release are within the max allocated resources that the
banker contains.
------------------------------------------------------------------------------*/
int
release_output () {
	/* Now checking to see if the client's claim was too high */
	/* If it was too high, an appropriate message is outputted */
	if (rel_check == 1) {
		printf("#    %d-Rel-Failed, senderID: %d, ", info.counter,
				bankerID);
		printf(" resources: { %2d %2d %2d %2d %2d  }\n", 
				msg.release[0], msg.release[1], msg.release[2], msg.request[3],
				msg.release[4]);
		printf("%s", line);
		printf("\tAvailable[]    = ( ");
		for(i = 0; i < info.res_type; i++) {
			printf("%2d ", info.available[i]);
		}
		printf("  )\n");
		success_output();
		printf("%s", line);
	} // end if
	else {
		printf("#    %d-Rel-Success, senderID: %d, ", info.counter,
				bankerID);
		printf("  resources: { %2d %2d %2d %2d %2d  }\n",
				msg.release[0], msg.release[1], msg.release[2], msg.release[3],
				msg.release[4]);
		success_output();
	} // end else
	return 0;
} // end release_output

/*------------------------------------------------------------------------------
request_output - This method is used to print out whether the banker has granted
, deemed the client's resources excessive, or the resources are unavailable to
prevent a deadlock from occurring with the resources.
------------------------------------------------------------------------------*/
int
request_output () {
	/* Now checking to see if the client's claim was too high */
	/* If it was too high, an appropriate message is outputted */
	if (compare == 1) {
		printf("#    %d-Excessive, senderID: %d, ", info.counter,
				bankerID);
		printf(" resources: { %2d %2d %2d %2d %2d  }\n", 
				msg.request[0], msg.request[1], msg.request[2], msg.request[3],
				msg.request[4]);
		printf("%s", line);
		printf("\tAvailable[]    = ( ");
		for(i = 0; i < info.res_type; i++) {
			printf("%2d", info.available[i]);
		} // end for
		printf("  )\n");
		success_output();
		printf("%s", line);
	} // end if
	else {
		printf("#    %d-Granted, senderID: %d, ", info.counter,
				bankerID);
		printf("  resources: { %2d %2d %2d %2d %2d  }\n",
				msg.request[0], msg.request[1], msg.request[2], msg.request[3],
				msg.request[4]);
		printf("%s", line);
		printf("\tAvailable[]    = ( ");
		for(i = 0; i < info.res_type; i++) {
			printf("%2d", info.available[i]);
		} // end for
		printf("  )\n");
		success_output();
		printf("%s", line);
	} // end else
	return 0;
} // end request_output

/*------------------------------------------------------------------------------
success_output - This method is used to print out the filled arrays after the 
initial requests have been decremented to fit within the maximum resources that
the banker contains.
------------------------------------------------------------------------------*/
int
success_output () {
	/* Local Variables */
	int ii;
	int ll;
	int x;
	int y;
	int temp_ID = 1;

	/* Switch/Case formatting the output */
	/* for allocated & need array's data */
	switch (msg.clientID) {
		case 1:
			/* Assignments */
			x = 0;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry data in needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing out data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 2:
			/* Assignments */
			x = 1;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Printing out data in arrays */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing out data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 3:
			/* Assignments */
			x = 2;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry data in needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 4:
			/* Assignments */
			x = 3;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry data in needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 5:
			/* Assignments */
			x = 4;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry data in needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing out data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 6:
			/* Assignments */
			x = 5;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry in needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing out data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 7:
			/* Assignments */
			x = 6;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry in needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y back to zero
			
			/* Printing out data in arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 8:
			/* Assignments */
			x = 7;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry to needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and 
			y = 0; // y back to zero
			
			/* Printing out arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 9:
			/* Assignments */
			x = 8;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry to needs array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("  )\n");
			} // end if
			
			x = 0; // Reassigning both x and
			y = 0; // y to zero
			
			/* for loop for printing arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
		case 10:
			/* Assignments */
			x = 9;
			y = 0;
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
				printf("\tAvailable[]    = ( ");
			} // end if
			
			/* Storing registry in the need array */
			for (ii = 0; ii < info.res_type; ii++) {
				needs[x][y] = msg.res[ii];
				if (msg2.purpose == 4) {
					alloc[x][y] = msg.request[ii];
					msg.allocated[ii] = alloc[x][y];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end if
				else if (msg2.purpose == 7) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
					alloc[x][y] = alloc[x][y] - msg.release[ii];
					info.available[ii] = info.available[ii] + msg.release[ii];
					printf("%2d ", info.available[ii]);
				} // end else if
				else if (msg2.purpose == 8 || msg2.purpose == 11) {
					alloc[x][y] = msg.allocated[ii];
					needs[x][y] = needs[x][y] - alloc[x][y];
					msg.need[ii] = needs[x][y];
				} // end else if
				y++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf(" )\n");
			} // end if
			
			x = 0; // Reassigning both the x and
			y = 0; // y back to 0 preventing errors
			
			/* for loop for printing out arrays */
			for (ii = 0; ii < MAX_CLIENTS; ii++) {
				if (temp_ID == 10) {
					printf("Allocated[ %d] = (", temp_ID);
				} // end if
				else {
				    printf("Allocated[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", alloc[x][y]);
					y++;
				} // end for
				y = 0;
				if (temp_ID == 10) {
					printf(" ) Need[ %d] = (", temp_ID);
				} // end if
				else {	
					printf(" ) Need[ %d]  = (", temp_ID);
				} // end else
				for (ll = 0; ll < STATIC_RESOURCE; ll++) {
					printf(" %d", needs[x][y]);
					y++;
				} // end for
				printf("  )\n");
				x++;
				y = 0;
				temp_ID++;
			} // end for
			
			/* Only print when Rel-Success */
			if (msg2.purpose == 7) {
				printf("-----%s", line);
			} // end if
			break;
	} // end switch/case
	return 0;
} // end success_output

/*------------------------------------------------------------------------------
Main - Runs the complete program of the banker
------------------------------------------------------------------------------*/
int
main () {
	/* Calls the two methods to begin */
	read_file();
	banker_process();
    
	return 0;
} // end main
