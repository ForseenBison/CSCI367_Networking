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

#include <stdbool.h>
#include <errno.h>
#include "trie.h"

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
	struct sockaddr_in cad1; /* structure to hold client's address */
	struct sockaddr_in cad2;
	int sd, sd2, sd3; /* socket descriptors */
	int port; /* protocol port number */
	int alen1; /* length of address */
	int alen2;
	int optval = 1; /* boolean value when we set socket option */
	char * buf = NULL;
	size_t length=0;
	uint8_t board_size; /* unsigned int to be the size of the board */
	uint8_t time_limit; /* unisgned int that determines how long players have to respond */

	if( argc != 5) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port board_size seconds_per_round dictionary_text_file\n");
		exit(EXIT_FAILURE);
	}

        /* Command-line Argument validation */
	
	board_size = strtoul(argv[2], NULL, 0);
	//null terminator size check stuff
	time_limit = strtoul(argv[3], NULL, 0);
	
	
	FILE *dictionary = fopen(argv[4], "r");
	struct TrieNode *root = getNode();


	if(dictionary == NULL)
	{
		fprintf(stderr, "Error: Could not open dictionary file");
		exit(EXIT_FAILURE);
	}

	while(getline(&buf, &length, dictionary)!= -1)
	{
		int i=0;
		while(buf[i]!='\n')
		{
			i++;
		}
		buf[i]='\0';
		insert(root, buf);	
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
	signal(SIGPIPE, SIG_IGN);
	/* Main server loop - accept and handle requests */
	while (1)
	{
	        
		int signalCheck=1;

		alen1 = sizeof(cad1);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad1, &alen1)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		
		char playerNum='1';
		send(sd2, (void *) &playerNum, sizeof(char), 0);
		send(sd2, (void *) &board_size, sizeof(uint8_t), 0);
		signalCheck = send(sd2, (void *) &time_limit, sizeof(uint8_t), 0);

		if(signalCheck<1)
                {
                        fprintf(stderr,"Connection Closed");
                        close(sd2);
			exit(EXIT_FAILURE);
                }


		alen2 = sizeof(cad2);
		if ( (sd3=accept(sd, (struct sockaddr *)&cad2, &alen2)) <0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

		playerNum='2';
                send(sd3, (void *) &playerNum, sizeof(char), 0);
		send(sd3, (void *) &board_size, sizeof(uint8_t), 0);
                signalCheck=send(sd3, (void *) &time_limit, sizeof(uint8_t), 0);

		if(signalCheck<1)
                {
                        fprintf(stderr,"Connection Closed");
                        close(sd2);
			close(sd3);
                        exit(EXIT_FAILURE);
                }

		pid_t p = fork();
     		
		if(p < 0) /*fork has failed*/
	        {
		  fprintf(stderr, "fork failed\n");
		  close(sd2);
		  close(sd3);
		}
		else if(p > 0) /* if parent, close sd2 and return to accept loop*/
		{
		  close(sd2);
		  close(sd3);
		}
		else if (p == 0) /* if child, play game with client */
		{
			//setting time limits
			struct timeval tv;
        		tv.tv_sec = time_limit;
        		tv.tv_usec = 0;

			if(setsockopt(sd2, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
						sizeof(struct timeval)) < 0)
        		{
                		fprintf(stderr, "Error Setting socket option failed\n");
                		exit(EXIT_FAILURE);
        		}

			if(setsockopt(sd3, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
                                                sizeof(struct timeval)) < 0)
                        {
                                fprintf(stderr, "Error Setting socket option failed\n");
                                exit(EXIT_FAILURE);
                        }


			uint8_t p1Wins=0;
			uint8_t p2Wins=0;
			uint8_t roundNum=0;
			
			//End of Game Check
			while(p1Wins < 3 && p2Wins < 3)
			{
				roundNum++;

				//Player 1's Scores
				send(sd2, (void *) &p1Wins, sizeof(uint8_t), 0);
				send(sd3, (void *) &p1Wins, sizeof(uint8_t), 0);

				//Player 2's Scores
				send(sd2, (void *) &p2Wins, sizeof(uint8_t), 0);
               	                send(sd3, (void *) &p2Wins, sizeof(uint8_t), 0);

				signalCheck=send(sd2, (void *) &roundNum, sizeof(uint8_t), 0);

				if(signalCheck<1)
                		{
                        		fprintf(stderr,"Connection Closed");
                        		close(sd2);
					close(sd3);
                       			exit(EXIT_FAILURE);
                		}

				signalCheck = send(sd3, (void *) &roundNum, sizeof(uint8_t), 0);

				if(signalCheck<1)
                		{
                		        fprintf(stderr,"Connection Closed");
                        		close(sd2);
					close(sd3);
                        		exit(EXIT_FAILURE);
                		}
			

				//Board Generation
				char board[board_size+1];
				int i;
				bool vowel=false;
				int temp=0;
				for(i=0; i< board_size; i++)
				{
					temp=rand()%26+97;	
					if(temp==97 || temp == 101 || temp == 105 ||
							temp == 111 || temp == 117)
					{
						vowel=true;
					}
	
					board[i]=temp;
				}
					
				//vowel check
				if(vowel)
				{
					temp=rand()%26+97;
					board[board_size]=temp;
				}
				else
				{
					while(!vowel)
					{
						temp=rand()%26+97;
						if(temp==97 || temp == 101 || temp == 105 ||
								temp == 111 || temp == 117)
                                		{
       	                        	        	vowel=true;
               	        	        	}
											}
					board[board_size-1]=temp;
				}
				//Server sends both Clients the board -- DON'T FORGET NETOWORK TO HOST CONVERSION
				signalCheck = send(sd2, (void *) &board, sizeof(char)*(board_size), 0);
				if(signalCheck<1)
                		{
                        		fprintf(stderr,"Connection Closed");
                        		close(sd2);
					close(sd3);
                        		exit(EXIT_FAILURE);
                		}

				signalCheck = send(sd3, (void *) &board, sizeof(char)*(board_size), 0);
				if(signalCheck<1)
                		{
                        		fprintf(stderr,"Connection Closed");
                        		close(sd2);
					close(sd3);
                        		exit(EXIT_FAILURE);
                		}
						
				int turnCheck=1;
				uint8_t turnNum=0;
				char yes = 'Y';
				char no = 'N';
				while(turnCheck)
				{
					struct TrieNode *wordsUsedThisRound = getNode();
					uint8_t currentPlayer = (roundNum+turnNum)%2;
					turnNum++;
					uint8_t sizeOfInput=0;
					int timeoutCheck=0;
					int validAnswer=0;
					char answer[255];

					if(currentPlayer==1)//sd2 is active
					{
						signalCheck = send(sd2, (void *) &yes, sizeof(char), 0);
						if(signalCheck<1)
                				{
                        				fprintf(stderr,"Connection Closed");
                        				close(sd2);
							close(sd3);
                        				exit(EXIT_FAILURE);
                				}
						
						signalCheck = send(sd3, (void *) &no, sizeof(char), 0);
						if(signalCheck<1)
                                                {
                                                        fprintf(stderr,"Connection Closed");
                                                        close(sd2);
							close(sd3);
                                                        exit(EXIT_FAILURE);
                                                }
						
						//Read in guess from sd2
						timeoutCheck = recv(sd2, (void *) &sizeOfInput, sizeof(uint8_t), 0);
						
						if(timeoutCheck==-1 && errno == EAGAIN)
						{
							//read timed out
							timeoutCheck=0;
							
						}
						else
						{
							//read did no time out
							timeoutCheck=1;
							recv(sd2, (void *) &answer, sizeof(char)*sizeOfInput, 0);

							answer[sizeOfInput]='\0';
							
							if(search(root, answer))
							{	
								
								int boardCheck=0;
								
								int answerCounter;
								int boardCounter;
								char tempAnswer[255];
								char tempBoard[board_size+1];
								strcpy(tempAnswer, answer);
								strcpy(tempBoard, board);

								for(answerCounter=0; answerCounter<sizeOfInput-1; answerCounter++)
								{
									for(boardCounter=0; boardCounter<board_size; boardCounter++)
									{
										if(tempBoard[boardCounter]==tempAnswer[answerCounter])
										{
											tempBoard[boardCounter]='-';
											boardCounter=0;
											answerCounter++;
										}
									}
									if(tempAnswer[answerCounter]!='\0')
									{
										boardCheck=0;	
									}
									else
									{
										boardCheck=1;
									}
								}

								if(boardCheck)
								{
									if(!search(wordsUsedThisRound, answer))
									{
										validAnswer=1;
										insert(wordsUsedThisRound, answer);
									}
									else
									{
										validAnswer=0;
										//Used Already
									}
								}
								else
								{
									validAnswer=0; //Not on Board
								}
							}
							else
							{
								validAnswer=0; //Not in dictionary
							}
						}
						
						//printf(".%u.\n", validAnswer);

						if(timeoutCheck && validAnswer)
						{
							//if valid send 1 to both players
							uint8_t temp=1;
							signalCheck = send(sd2, (void *) &temp, sizeof(uint8_t), 0);
							if(signalCheck<1)
	                                                {
        	                                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
                        	                                exit(EXIT_FAILURE);
                                	                }
							
							signalCheck = send(sd3, (void *) &temp, sizeof(uint8_t), 0);
							if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

                                                        //send the word to inactive player
							send(sd3, (void *) &sizeOfInput, sizeof(uint8_t),0 );
							signalCheck = send(sd3, (void *) &answer, sizeof(char)*sizeOfInput, 0);
							if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

                                                        //repeat turn loop
							turnCheck=1;
						}
						else
						{
							//if answer isn't valid send 0 to both players
							uint8_t temp=0;
							signalCheck = send(sd2, (void *) &temp, sizeof(uint8_t), 0);
							if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

							signalCheck = send(sd3, (void *) &temp, sizeof(uint8_t), 0);
							if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

                                                        //exit turn loop
							turnCheck=0;
                                                        //inactive player's score is incremented by one
							p2Wins++;
						}
					}
					else //sd3 is active
					{
						signalCheck = send(sd3, (void *) &yes, sizeof(char), 0); 
                                                if(signalCheck<1)
                                                {
                                                        fprintf(stderr,"Connection Closed");
                                                        close(sd2);
							close(sd3);
                                                        exit(EXIT_FAILURE);
                                                }

						signalCheck =send(sd2, (void *) &no, sizeof(char), 0);
						if(signalCheck<1)
                                                {
                                                        fprintf(stderr,"Connection Closed");
                                                        close(sd2);
							close(sd3);
                                                        exit(EXIT_FAILURE);
                                                }

						//Read in guess form sd3
						timeoutCheck = recv(sd3, (void *) &sizeOfInput, sizeof(uint8_t), 0);
						
						if(timeoutCheck==-1 && errno == EAGAIN)
                                                {
                                                        //read timed out
                                                        timeoutCheck=0;

                                                }
                                                else
                                                {
                                                        //read did no time out
                                                        timeoutCheck=1;
                                                        recv(sd3, (void *) &answer, sizeof(char)*sizeOfInput, 0);

                                                        answer[sizeOfInput]='\0';

                                                        if(search(root, answer))
                                                        {
                                                                if(!search(wordsUsedThisRound, answer))
                                                                {
                                                                        validAnswer=1;
                                                                        insert(wordsUsedThisRound, answer);
                                                                }
								else
								{
									validAnswer=0;
								}
                                                        }
                                                        else
                                                        {
                                                                validAnswer=0;
                                                        }
                                                }

                                                if(timeoutCheck && validAnswer)
                                                {
                                                        //if valid send 1 to both players
                                                        uint8_t temp=1;
                                                        signalCheck = send(sd2, (void *) &temp, sizeof(uint8_t), 0);
                                                        if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

							signalCheck = send(sd3, (void *) &temp, sizeof(uint8_t), 0);
							if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

                                                        //send the word to inactive player
                                                        send(sd2, (void *) &sizeOfInput, sizeof(uint8_t), 0);
                                                        signalCheck = send(sd2, (void *) &answer, sizeof(char)*sizeOfInput, 0);

                                                        if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

							//repeat turn loop
                                                        turnCheck=1;
                                                }
                                                else
                                                {
                                                        //if answer isn't valid send 0 to both players
                                                        uint8_t temp=0;
                                                        signalCheck = send(sd2, (void *) &temp, sizeof(uint8_t), 0);
                                                        if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

							signalCheck = send(sd3, (void *) &temp, sizeof(uint8_t), 0);
                                                        if(signalCheck<1)
                                	                {
                        	                                fprintf(stderr,"Connection Closed");
                	                                        close(sd2);
								close(sd3);
        	                                                exit(EXIT_FAILURE);
	                                                }

							//exit turn loop
                                                        turnCheck=0;
							 //inactive player's score is incremented by one
                                                        p1Wins++;
                                                }
					}//End of Sd3 is active

				}//End of turn loop

			}//Exit of Round loop
			
			

			close(sd2);
			close(sd3);
			exit(EXIT_SUCCESS);
		}
	}
}

