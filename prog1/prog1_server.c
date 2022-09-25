#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/ioctl.h>

#define QLEN 6 /* size of request queue */

/*------------------------------------------------------------------------
* Program: Prog1 - Hangman
*
* Purpose: 
* (1) wait for the next connection from a client
* (2) connect and fork the processes 
* (3) child processes sends and recieves information from client for 
*     hangman game, exits when game is finished
* (4) parent processes goes back to step (1)
*
* Syntax: ./prog1_server port tot_guesses word
*
* port - protocol port number to use
* tot_guesses - guesses allowed in the game, between 0 and 26 (non-inclusive)
* word - word to be guessed by player, must contain only lowercase letters
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	char buf[1000]; /* buffer for string the server sends */
	uint8_t guesses; /* unsigned int to hold allowed guesses */
	uint8_t wlength; /* unisgned int to hold length of the word */
	char* word; /* pointer to the word for guessing */
	char* wprogress; /*pointer to a "board", starts as all underscores 
			   length of the word and fills in with corresponding
			   letters if player guesses correctly */

	if( argc != 4) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port guesses_permited word\n");
		exit(EXIT_FAILURE);
	}

	if(atoi(argv[2]) < 1 || atoi(argv[2]) > 25)
	{
	  fprintf(stderr, "Error: Wrong number of guesses\n");
	  fprintf(stderr, "usage: 0 < guesses < 26\n");
	  exit(EXIT_FAILURE);
	}

        guesses = strtoul(argv[2], NULL, 0);
	
	/* contents of the word are validated to be
	 * lowercase letters and the length of the word
	 * is counted */        	
	word = argv[3];
	int i = 0;
	while(word[i] != 0)
	{
	  if(word[i] < 97 || word[i] > 122)
	  {
	    fprintf(stderr, "Error: invalid word\n");
	    fprintf(stderr, "usage: word must be all lowercase letters.\n");
	    exit(EXIT_FAILURE);
	  }

	  i++;
	}

	if(i == 1 || i > 253)
	{
	  fprintf(stderr, "Error: invalid word\n");
	  fprintf(stderr, "usage: word length N must b 1 < N < 254 letters\n");
	  exit(EXIT_FAILURE);
	}

	wlength = i;

	/* the word in progress is initialized 
	 * to be length of the word and starts 
	 * as all '_' */
	wprogress = malloc((i+1)*sizeof(char));
	if(wprogress == NULL)
	{
	  fprintf(stderr, "Malloc failed\n");
	  exit(EXIT_FAILURE);
	}

	i = 0;
	wprogress[wlength] = 0;
	while(i < wlength)
	{
	    wprogress[i] = '_';
	    i++;
	}
	
	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, SIG_IGN);
	/* Main server loop - accept and handle requests */
	while (1) {
	        alen = sizeof(cad);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

		pid_t p = fork();
     		
		if(p < 0) /*fork has failed*/
	        {
		  fprintf(stderr, "fork failed\n");
		  close(sd2);
		}
		else if(p > 0) /* if parent, close sd2 and return to accept loop*/
		{
		  close(sd2);
		}
		else if (p == 0) /* if child, play game with client */
		{

		  int mode =0;
		  ioctl(sd2, FIONBIO, &mode);		
				  
		  write(sd2, &wlength, sizeof(wlength)); /* send length of the word */

		  int notComplete = 1;
		  int letterFound = 0;
		  char guess;

		  write(sd2,&guesses, sizeof(guesses)); /* send number of guesses */
		  /* while there are guesses left, and the word has not been found
		   * play the game with client */
		  while(guesses > 0 && notComplete == 1)
		  {		    
		    write(sd2, wprogress, wlength); /* send the word in progress */
		    read(sd2, &guess, sizeof(char)); /* read the character guesses */
			
		    /*while loop to find if the player guessed a correct unfound letter */
		    letterFound = 0;
		    int ix = 0;
		    while(word[ix] != 0)
		    {
		      if((word[ix] == guess) && (guess != '$'))
		      {
			wprogress[ix] = word[ix];
			word[ix]='$';
			letterFound = 1;
		      }
		      ix++;
		    }
		  
		    /* while loop to check if the player eliminated all '_' characters,
		     * if none are found the player has won */
		    ix = 0;
		    notComplete = 0;
		    while(ix < wlength)
		    {
		      if(wprogress[ix] == '_') notComplete = 1;
		      ix++;
		    }
			
		    /* player has won, setting guesses to 255 
		     * so client knows it has won */
		    if(notComplete == 0)
		    {
		      guesses = 255;
		    }
		  
		    /* player didn't find any letters, decrement guesses */
		    if(letterFound == 0) guesses--;

		    /*send guesses to client*/
		    write(sd2, &guesses, sizeof(guesses));
		  }
		
		  /* send final board to client
		   * close, and exit */
		  write(sd2, wprogress, wlength);
		  close(sd2);
		  exit(EXIT_SUCCESS);
		}
	}
}

