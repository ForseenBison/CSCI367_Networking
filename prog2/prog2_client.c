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
	
	char playerNum='0';
	uint8_t board_size=0;
	uint8_t time_limit=0;

	recv(sd, (void *) &playerNum, sizeof(char), 0);
	recv(sd, (void *) &board_size, sizeof(uint8_t), 0);
	recv(sd, (void *) &time_limit, sizeof(uint8_t), 0);

	printf("You are player %c!\n", playerNum);
	printf("The Number of Letters on the board for this game is: %u\n", board_size);
	printf("The time limit on turns for this game is: %u seconds.\n", time_limit);
	if (playerNum=='1')
	{
		printf("Waiting for player 2 to connect...\n");
	}
	else
	{
		printf("Connecting...\n");
	}

	//Game Loop starts
	uint8_t myScore=0;
	uint8_t oppScore=0;
	uint8_t roundNum=0;
	char board[board_size+1];

	while(myScore < 3 && oppScore < 3)
	{
		if(playerNum=='1')
		{
			recv(sd, (void *) &myScore, sizeof(uint8_t), 0);
			recv(sd, (void *) &oppScore, sizeof(uint8_t), 0);
		}
		else
		{
			recv(sd, (void *) &oppScore, sizeof(uint8_t), 0);
                        recv(sd, (void *) &myScore, sizeof(uint8_t), 0);
		}

		recv(sd, (void *) &roundNum, sizeof(uint8_t), 0);
		recv(sd, (void *) &board, sizeof(char)*(board_size), 0);
		board[sizeof(char)*board_size]='\0';

		printf("Score: %u-%u\n", myScore, oppScore);
		printf("Round #%u\n", roundNum);
		printf("This Rounds Board:\n%s\n", board);

		int turnCheck=1;
		char activePlayer = 'N';
		while(turnCheck)
		{
			recv(sd, (void *) &activePlayer, sizeof(char), 0);

			if(activePlayer=='N')//Inactive
			{
				uint8_t validCheck=0;
				printf("It's the Other Players Turn!\n");

				recv(sd, (void *) &validCheck, sizeof(uint8_t), 0);

				if(validCheck==0)
				{
					printf("The Other player made a mistake! You win this round!\n");
					turnCheck=0;
				}
				else
				{
					char cResponse[255];
					uint8_t numRead=0;
					recv(sd, (void *)  &numRead, sizeof(uint8_t), 0);
					recv(sd, (void *) &cResponse, sizeof(char)*numRead, 0);
					cResponse[numRead]='\0';
					printf("The Other Player's word: %s, was vaild. Your turn next.\n", cResponse);
				}
			}
			else //Active
			{
				uint8_t validCheck=0;
				printf("It's Your turn!\n");
				char *answer=malloc(sizeof(char)*255);
				printf("Please give a valid word from the Board: ");
				//int numRead = getline(&answer, &size, stdin);
				fgets(answer, 255, stdin);	
				uint8_t numRead=0;
				while(answer[numRead]!='\n')
				{
					numRead++;
				}

				
				//Send size
				send(sd, (void *) &numRead, sizeof(uint8_t), 0);
				
				//Send Response
				send(sd, (void *) answer, sizeof(char)*numRead, 0);
				
				//Recive Valid Check
				recv(sd, (void *) &validCheck, sizeof(uint8_t), 0);

				if(validCheck==0)
				{
					printf("Your word was invalid. The other player wins this round :(\n");
					turnCheck=0;
				}
				else
				{
					printf("Your word was valid. Now for the other player.\n");
				}
			}
		}
				
	}

	if(myScore>oppScore)
	{
		printf("You Win!\n");
	}
	else
	{
		printf("You Lose :(\n");
	}

	close(sd);
	exit(EXIT_SUCCESS);
}

