#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/ioctl.h>

/*------------------------------------------------------------------------
* Program: Prog3_observer chat client
* Purpose: Connect to a server hosting chat client and recv messages from the server and display
*
* Syntax: ./prog3_observer server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/
int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Connected, ready to recv\n");
	int mode = 0;
	ioctl(sd, FIONBIO, &mode);
	
	char valid;
	recv(sd, (void *) &valid, sizeof(char), 0);
	/*TODO*/printf("Read %c\n", valid);

	if(valid=='N')
	{
		close(sd);
	}
	else //valid == Y
	{
		char loopCheck = 'T';
		
		while(loopCheck=='T')
		{
			char username[11];
			printf("Please enter a valid username, 1-10 characters, only letters, numbers, -, _. You have 60 seconds: \n");
			fgets(username, 12, stdin);
			uint8_t username_len=strlen(username)-1;
			/*TODO*/printf("Sending %i\n", username_len);
			send(sd, (void *) &username_len, sizeof(uint8_t), 0);
			/*TODO*/printf("Sending %s as %i\n", username, username_len);
			send(sd, (void *) &username, username_len, 0);

			int close_check = recv(sd, (void *) &loopCheck, sizeof(char), 0);
			if(close_check==0)
			{
				printf("The server has disconnnected from you, because of timeout, closing program");
				exit(EXIT_FAILURE);
			}
			
			if(loopCheck=='Y')
			{
				while(1)
				{
					uint16_t buffer_len;
					recv(sd, (void *) &buffer_len, sizeof(uint16_t), 0);

					char buffer[buffer_len+2];//+2 in case of needing to add newline
					
					recv(sd, (void *) &buffer, sizeof(uint16_t), 0);
					buffer[buffer_len+1]='\0';
					
					int i=0;
					while(buffer[i]!='\0' || buffer[i]!='\n') { i++; }
					
					buffer[i]='\n';
					buffer[i+1]='\0';
					
					printf("%s", buffer);
				}
			}
			else if(loopCheck=='N')
			{
				printf("\n\nInvalid Username, Disconnecting\n");
				close(sd);
				exit(EXIT_FAILURE);
			}
			else // LoopCheck == T
			{
				printf("\n\nThere is already an observer connected to that username, your timer has been reset, please try again.\n");
			}
		}
	}
	close(sd);
	exit(EXIT_SUCCESS);
}

