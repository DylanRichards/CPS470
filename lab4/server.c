#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include "threadpool.h"

#define MAX_CLIENTS 3
#define BUFFER_SZ 2048

static int uid = 1;
static _Atomic unsigned int cli_count = 0;
pthread_t tid[MAX_CLIENTS]; /* the thread identifier */

/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Receive a message from a socket
	recv(cli->sockfd, name, 32, 0);
	
	// Store client's name
	strcpy(cli->name, name);
	// Promote client joining message
	sprintf(buff_out, "<<<%s has joined>>>\n", cli->name);
	printf("%s", buff_out);
	

	// keep receiving client's messages
	while(leave_flag == 0){
		
		// Erases the data in buffer_out
		bzero(buff_out, BUFFER_SZ);
	
		// Receive a message from a socket
		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		
		// Receive sucessfully
		if (receive > 0){
			// Receive none empty message 
			if(strlen(buff_out) > 0){
				// Remove the newline character '\n' in the end of the message
				buff_out[strlen(buff_out)-1] = '\0';
				printf(">%s\n", buff_out);
			}
		// Check "exit" message
		} else if (receive == 0 || strncmp(buff_out, "exit", 4) == 0){
			sprintf(buff_out, "<<<%s has left>>>\n", cli->name);
			printf("%s", buff_out);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
	}

	/* Delete client from pool and yield thread */
	close(cli->sockfd);
	cli_count--;
	free(cli);
	
	return NULL;
}

int main(int argc, char **argv){

	if(argc != 2){
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0;
	int connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;

	// Socket settings 
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	// Get and set options on sockets */
	setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option));

	// Bind a name to a socket
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	// Listen for connections on a socket
	listen(listenfd, 10);

	printf("=== WELCOME TO THE CHATROOM ===\n");
	
	while(1){
		// Accept client connection 
		
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		// Client settings 
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;
		
		// Handle client connection	
		/* create the thread */
		work_submit(&handle_client, (void*)cli);
		// pthread_create(&tid[cli_count], NULL, &handle_client, (void*)cli);
	}

	return EXIT_SUCCESS;
}
