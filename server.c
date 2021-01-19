// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "common.h"

int main(int argc, char *argv[]) {
	struct sockaddr_in servaddr, cliaddr; 
	int sockfd, port, n, len, closed;
	FILE *fp;
	packet p = {0};
	int32_t ack;
	
    // usage
	if (argc < 2) {
        fprintf(stderr, "usage %s <recv_port>\n", argv[0]);
        exit(0);
    }

    // gather information
	port = atoi(argv[1]);

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
		error("socket creation failed");
	
	// create file to store data
	if ((fp = fopen("server_result.txt", "w")) < 0)
		error("couldn't create file for storage");

	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(port); 
	
	// Bind the socket with the server address 
	if ( bind(sockfd, (const struct sockaddr *)&servaddr, 
			sizeof(servaddr)) < 0 ) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	len = sizeof(cliaddr); 

	// start waiting for the file
	closed = 0;
	ack = 0;

	while (!closed) {
		// wait for packet
		n = recvfrom(sockfd, &p, sizeof(packet), 0, (struct sockaddr *) &cliaddr, &len);

		// store result and send acknowledgement
		if (p.seqN <= ack && p.len > 0 && p.len <= DATA_SIZE) {
			// got the needed packet to move on
			if (p.seqN == ack) {
				//printf("[Server] Recieved packet %d of length %d containing %s\n", p.seqN, p.len, p.data);
				fwrite(p.data, 1, p.len, fp);
				ack++;
			}
			// resend old acks incase they got lost somehow
			sendto(sockfd, &p.seqN, 4, 0, (const struct sockaddr *) &cliaddr, len);
		}

		// done with file
		if (p.seqN == EOF_SIGNAL) {
			printf("[Server] RECIEVED THE EOF_SIGNAL!!!\n");

			int final_ack = EOF_SIGNAL;
			sendto(sockfd, &final_ack, 4, 0, (const struct sockaddr *) &cliaddr, len);

			// CLOSE FILE
			if (!closed) {
				closed = 1;
				printf("[Server] FILE SAVED!\n");
				fclose(fp);
			}

			// WAIT FOR NEXT FILE
			ack = 0;
		}
	}

	printf("[Server] exited gracefully\n");
	return 0; 
} 