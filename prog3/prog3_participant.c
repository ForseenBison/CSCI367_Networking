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
* Program: Prog3_participant chat client
* Purpose: Connect to a server hosting chat client and send messages to the server
*
* Syntax: ./prog3_participant server_address server_port
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

	int mode = 0;
	ioctl(sd, FIONBIO, &mode);
	
	printf("Connected, ready to recv\n");
	
	char valid;
	recv(sd, (void *) &valid, sizeof(char), 0);
	if(valid=='N')
	{
		close(sd);
	}
	else
	{
		char loopCheck='T';
		while(loopCheck=='T' || loopCheck == 'N')
		{
			char username[11];
                	printf("Please enter a valid username, 1-10 characters, only letters, numbers, -, _. You have 60 seconds: \n");
                	fgets(username, 12, stdin);
                	uint8_t username_len=strlen(username)-1;

			/*TODO*/printf("Sending %i\n", username_len);
                	send(sd, (void *) &username_len, sizeof(uint8_t), 0);
			/*TODO*/printf("Sending %s as %i bytes\n", username, username_len);
        	        send(sd, (void *) &username, username_len*sizeof(char), 0);

                        int close_check = recv(sd, (void *) &loopCheck, sizeof(char), 0);
                        /*TODO*/printf("Read %c\n", loopCheck);
			if(close_check==0)
                        {
                                printf("\n\nThe server has disconnnected from you, because of timeout, closing program\n");
                                exit(EXIT_FAILURE);
                        }
			
			if(loopCheck=='Y')
			{
				while(1)
				{
					printf("Enter message: ");
					char buffer[1011];
					fgets(buffer, 1011, stdin);
					
					int i=0;
					int blankCheck=0;
					while(buffer[i]!='\0')
					{
						if(buffer[i]!=' ')
						{
							blankCheck=1;
							i=strlen(buffer);
						}
					}
					
					if(blankCheck)
					{
						uint16_t buffer_len= strlen(buffer);
						send(sd, (void *) &buffer_len, sizeof(uint16_t), 0);
						send(sd, (void *) &buffer, buffer_len, 0);
					}
					else
					{
						printf("\nWarning: message not sent b/c a message must contain at least one non-whitespace character\n");
					}
				}
				
			}
			else if(loopCheck=='I')
			{
				printf("\n\nThat username was invalid, please try again, your time has not been reset.\n");
			}
			else //loopCheck==T
			{
				printf("\n\nThat username is already taken, your timer has been reset, please try again\n");
			}
		}
	}
	close(sd);
	exit(EXIT_SUCCESS);
}

