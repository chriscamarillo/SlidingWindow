// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>
#include "common.h"

// Window management
typedef struct node {
    int sent;
    packet p;
    struct node* next;
} Node; 

struct node* pop_and_shift(struct node* head) {
    if (head == NULL)
        return NULL;

    struct node* next_head = head->next;
    free(head);
    return next_head;
}


void add_to_end(struct node* head, struct node* nodeToadd) {
    if (head != NULL) {
        struct node* prevNode = head;
        struct node* nextNode = head->next;

        while (nextNode != NULL) {
            prevNode = nextNode;
            nextNode = nextNode->next;
        }
        prevNode->next = nodeToadd;
        nodeToadd->next = NULL;
    }
}

// debug
void print_window(struct node* window_head) {
    struct node* current = window_head;
    packet p;

    while (current) {
        p = current->p;
        printf("[reqNo: %d\tlen: %d\tmessage: %s]\n", p.seqN, p.len, p.data);
        current = current->next;
    }
}

// File management
struct node* make_next_node(FILE* fp) {
    struct node* nextNode = (struct node *) malloc(sizeof(struct node));
    int len = 0;

    if ((len = fread(nextNode->p.data, 1, DATA_SIZE, fp)) > 0) {
        nextNode->sent = 0;
        nextNode->p.len = len;
        return nextNode;
    }
    free(nextNode);
    return NULL;
}

int main(int argc, char *argv[]) { 
	struct sockaddr_in servaddr;
	int sockfd, port, n, len, bytesRead;
	char *host, *filename;
    packet p = {0};
    int32_t seq = 0, ack = 0;
    struct node* window_head = malloc(sizeof(struct node));
    clock_t time;
    double elapsedMilli; 
    FILE* fp;

    // usage
    if (argc < 4) {
        fprintf(stderr, "usage %s <recv_host> <recv_port> <filename>\n", argv[0]);
        exit(0);
    }

    // Gather information
    host = argv[1];
    port = atoi(argv[2]);
    filename = argv[3];

    if ((fp = fopen(filename, "rb")) == NULL)
        error("[CLIENT] failed to open file!");
    
	// socket creation
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	    error("[CLIENT] failed to create socket");
    
    // Filling server information 
    memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(port); 
	servaddr.sin_addr.s_addr = inet_addr(host); 
	
    // start sharing
    // initial load
    struct node* dummy = window_head;
    struct node* current = window_head;
    int i = 0;

    while (i < WINDOW_SIZE && current != NULL) {
        current->next = make_next_node(fp);
        current = current->next;
        if (current) current->p.seqN = i;
        i++;
    }

    window_head = window_head->next;
    free(dummy);
    // DEBUG: print_window(window_head);
    
    // start
    time = clock();
    elapsedMilli = 0;

    // initial load 
    struct node* base = window_head;
    current = base;

    // concurrent control
    int wait = 0;
    
    bytesRead = 0;

    while (base) {
        // DEBUG: print_window(base);
        if (base->p.seqN > 2000)
            exit(0);
        current = base;

        // timing
        elapsedMilli = (clock() - time) * 1000 / (CLOCKS_PER_SEC);

        // check for inital sends
        while (current) {
            if (current->sent == FIRST_TRANSMISSION) {
                printf("[send data] start: %d (%d).\n", bytesRead, current->p.len); 
                current->sent = RETRANSMISSION;

                packet p = current->p;
	            sendto(sockfd, &current->p, sizeof(current->p), MSG_DONTWAIT, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            }
            current = current->next;
        }

	
	    n = recvfrom(sockfd, &ack, 4, MSG_DONTWAIT, (struct sockaddr *) &servaddr, &len);
        

        // mark successful sends
        if (ack == base->p.seqN && base->sent < SUCCESS) {
	        printf("[recv ack] ackno %d\n", ack);
            base->sent = SUCCESS;

            // slide the window
            bytesRead += base->p.len;
            base = pop_and_shift(base);
            wait = 1; // signal wait on window sender
            ack++;
            
            // append window if theres still pieces of the file to send
            struct node* append = make_next_node(fp);
            
            if (append) {
                append->p.seqN = ack;
                add_to_end(base, append);
            }
            
            // reset timer
            time = 0;
        }

        // resend window
        if (elapsedMilli > TIMEOUT_MS) {
            current = base;
            while (current) {
                printf("[resend data] start: %d (%d).\n", bytesRead, current->p.len);
                // RETRANSMISSION
                if (current->sent == 1)
                    sendto(sockfd, &current->p, sizeof(current->p), MSG_DONTWAIT, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                current = current->next;
            }
            time = clock();
            elapsedMilli = 0;
        }

    }
    // done tie up loose ends
    packet done;
    done.seqN = EOF_SIGNAL;
    done.len = EOF_SIGNAL;
    // hacky tacky
    for (int i = 0; i < 5; i++)
        sendto(sockfd, &done, sizeof(packet), MSG_DONTWAIT, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    printf("[Client] exited gracefully.\n");
    fclose(fp);
	close(sockfd); 
	return 0; 
} 
