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
* Program: Prog1_client hangman
* Purpose: Connect to a server hosting a hangman game and play the game
*
* Syntax: ./prog1_client server_address server_port
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
	int n; /* number of characters read */
	char buf[1000]; /* buffer for data from the server */

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
	
	/* recieve the wordlength from server */
	uint8_t wLength = 0;
	read(sd, &wLength, sizeof(wLength));

	printf("%i word Length\n", wLength);

	uint8_t guesses;
	char * board = malloc((wLength+1)*sizeof(char));
	board[wLength] = 0;
	char guess[2];

	/* read the number of guesses from server */
	read(sd, &guesses, sizeof(guesses));
	
	while(guesses != 0 && guesses != 255)
	{
	  
	  /* read the word in progress from the client */
	  read(sd, board, sizeof(char)*wLength);

	  printf("Guesses: %u\n", guesses);
	  printf("%s\n", board);
	  printf("Please enter a letter\n");
	  fgets(guess, 3, stdin); /*get player's guess*/
          
	  /* send guess to the server */
	  write(sd, &guess, sizeof(char));

	  /* recieve guesses from server */
	  read(sd, &guesses, sizeof(guesses));
	}
        
	/* read the final board */
	read(sd, board, sizeof(char)*wLength);
	
	/* if guesses is 255 the player has won */
	if(guesses == 255)
	{
	  printf("You won!\n");
	}
	else /*otherwise they lost */
	{
	  printf("You lost.\n");
	}

	/* print the final board */
	printf("Final Board: %s\n", board);
	
	close(sd);
	exit(EXIT_SUCCESS);
}

