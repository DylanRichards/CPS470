#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

// replace \n with NULL
void str_trim_lf (char* arr, int length) {
	int i;
	for (i = 0; i < length; i++) { // trim \n
		if (arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}

// accept user message and send it to server
void send_msg_handler() {
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};
	
	while(1) {
		printf(">");
		fgets(message, LENGTH, stdin);
		
		if (strncmp(message, "exit", 4) == 0) {
			break;
		} else {
			strcpy(buffer, name);
			strcat(buffer, ": ");
			strcat(buffer, message);
			send(sockfd, buffer, strlen(buffer), 0);
		}
		// Empty buffer
		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}						
}


int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr,"Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // Use local host as server address
	int port = atoi(argv[1]); // Accept user input as port number

	printf("Please enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name)); // replace \n with NULL


	if (strlen(name) > 32 || strlen(name) < 2){
		fprintf(stderr,"Name must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr; // Address family for client

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	// Connect to Server
	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
		fprintf(stderr, "Can't connect to server using port %d.\n", port);
	}

	// Send name to server
	else if(send(sockfd, name, 32, 0)==-1){
		fprintf(stderr, "Can't connect to server using port %d.\n", port);
	}
	
	else{
		printf("=== WELCOME TO THE CHATROOM ===\n");

		// Handle user messaeg
		send_msg_handler();

		// close socket
		close(sockfd);
	}

	return EXIT_SUCCESS;
}
