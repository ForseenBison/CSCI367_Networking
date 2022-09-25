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

//Personal Includes
#include <ctype.h>
#include <time.h>

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

typedef struct Node
{
	char username[11];
	int sd;
	struct Node *next;
	int active;
	int participant;

} Node;

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct protoent *ptrp2;	

	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in sad2;//sad is for participant and sad2 is for observer
	
	struct sockaddr_in oad; /* structure to hold observer's address */
	struct sockaddr_in pad; /* structure to hold participant's address*/
	
	int sdp, sdo, sd2; /* socket descriptors */

	int participantPort; /* protocol participant port number */
	int observerPort; /* protocol observer port number */

	int alen; /* length of address */
	int alen2;
	int optval = 1; /* boolean value when we set socket option */

	fd_set participants;
	FD_ZERO(&participants);
	int num_FD = 0;
	int num_observer=0;	

        Node *root = NULL;
	//root = malloc(sizeof(struct Node));
	

	if( argc != 3) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server participant_port observer_port\n");
		exit(EXIT_FAILURE);
	}
	
	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	memset((char *)&sad2,0,sizeof(sad2));


	sad.sin_family = AF_INET; /* set family to Internet */
	sad2.sin_family = AF_INET;

	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */
	sad2.sin_addr.s_addr = INADDR_ANY;

	participantPort = atoi(argv[1]); /* convert argument to binary */
	observerPort = atoi(argv[2]);


	if (participantPort > 0)
	{ /* test for illegal value */
		sad.sin_port = htons((u_short)participantPort);
	}
	else
	{ /* print error message and exit */
		fprintf(stderr,"Error: Bad participant port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	if (observerPort > 0)
	{
		sad2.sin_port = htons((u_short)observerPort);
	}
	else
	{
		fprintf(stderr,"Error: Bad observer port number %s\n",argv[2]);
                exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
        if ( ((long int)(ptrp2 = getprotobyname("tcp"))) == 0) {
                fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
                exit(EXIT_FAILURE);
        }


	/* Create a socket */
	sdo = socket(PF_INET, SOCK_STREAM, ptrp2->p_proto);
	if (sdo < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	sdp = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sdp < 0)
	{
		fprintf(stderr, "Error: Socket creation failed\n");
                exit(EXIT_FAILURE);
	}


	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sdo, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	if( setsockopt(sdp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
                fprintf(stderr, "Error Setting socket option failed\n");
                exit(EXIT_FAILURE);
        }

	/* Bind a local address to the socket */
	if (bind(sdo, (struct sockaddr *)&sad2, sizeof(sad2)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	if (bind(sdp, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
                fprintf(stderr,"Error: Bind failed\n");
                exit(EXIT_FAILURE);
        }

	/* Specify size of request queue */
	if (listen(sdo, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sdp, QLEN) < 0) {
                fprintf(stderr,"Error: Listen failed\n");
                exit(EXIT_FAILURE);
        }

	FD_ZERO(&participants);
	FD_SET(sdo, &participants);
	FD_SET(sdp, &participants);
	int max_FD = 0;
	if(sdo>sdp)
	{
		max_FD = sdo+1;
	} 
	else
	{
		max_FD = sdp+1;
	}
	num_participants=0;
	num_observer=0;

	signal(SIGCHLD, SIG_IGN);
	/* Main server loop - accept and handle requests */
	while (1)
	{
	        alen = sizeof(oad); 
		alen2 = sizeof(pad);


		//select
		int select_return = -1;
		pid_t p;
		int FD_flag = 0; //1 is new observer and 2 is new participant and 3 is active participant.
		/*TODO*/printf("about to hit select\n");	
		select_return = select(max_FD, &participants,NULL,NULL,NULL);
		/*TODO*/printf("Hitting Select, %i\n", select_return);

		if(select_return == -1)
		{
			fprintf(stderr,"Select error");

		}
		else if (FD_ISSET(sdo, &participants)) //new observer
		{
			/*TODO*/printf("Connecting new Observer\n");
			if ( (sd2=accept(sdo, (struct sockaddr *)&oad, &alen)) < 0)
			{
                        	fprintf(stderr, "Error: Accept failed\n");
                       		exit(EXIT_FAILURE);
			}
			
			char valid ='N';
                        if(num_observers < 255)
                        {
                                valid='Y';

                                Node *new_Node = malloc(sizeof(struct Node));
                                new_Node->active=0;
                                new_Node->username="";
                                new_node->participant=1;
                                new_node->sd=sd2;

                                Node trv=root;
                                if(trv==NULL)
                                {
                                        root=new_Node;
                                }
                                else
                                {
                                        while(trv->next != NULL)
                                        {
                                                trv=trv->next;
                                        }
                                        trv->next = new_Node;
                                }

                                if(new_Node->sd>max_FD-1)
                                {
                                        max_FD=new_Node->participant_sd+1;
                                }
                        }
                        send(sd2, (void *) &valid, sizeof(char), 0);
			if(valid=='N')
			{
				close(sd2);
			}

                }
		else if (FD_ISSET(sdp, &participants)) //new participant
		{
			/*TODO*/printf("Connecting new Participant\n");
			if ( (sd2=accept(sdp, (struct sockaddr *)&pad, &alen2)) < 0)
			{
                       		fprintf(stderr, "Error: Accept failed\n");
                        	exit(EXIT_FAILURE);
			}
			char valid ='N';
                        if(num_observer < 255)
                        {
                        	valid='Y';
			
				Node *new_Node = malloc(sizeof(struct Node));
				new_Node->active=0;
				new_Node->username="";
				new_node->participant=1;
				new_node->sd=sd2;

				Node trv=root;
				if(trv==NULL)
				{
					root=new_Node;
				}
				else
				{
					while(trv->next != NULL)
                                	{
                        	        	trv=trv->next;
                	                }
        	                        trv->next = new_Node;
				}
			
				if(new_Node->sd>max_FD-1)
                	        {
        		                max_FD=new_Node->participant_sd+1;
				}
			}
			send(sd2, (void *) &valid, sizeof(char), 0);
			if(valid=='N')
			{
				close(sd2);
			}
                }
		else //it's not a listening socket
		{
			/*TODO*/printf("something that isn't a listening socket detected\n");
			
			Node trv=root;
			while(!FD_ISSET(trv.sd, &participants) && trv!=NULL)
			{
				trv=trv.next;
			}

			if(trv==NULL)
			{
				//i guess we recived a signal from not something real, i dunno it's bad
				exit(EXIT_FAILURE);
			}
			
			if(trv.active==0) //Username inc
			{
				if(trv.participant==0) //inactive observer
				{
					
				}
				else //inactive participant
				{
					
				}
			}
			else//active == 1
			{
				if(trv.participant==0) //active observer [i think this is a disconnect]
				{
						
				}
				else //active participant
				{
					
				}
			}
		}
	}
}
		//check forking!
		if(p < 0 ) // fork has failed
		{
			fprintf(stderr, "forking has failed");
			close(sd2);
		}
		else if(p> 0)// if parent close sd2
		{
			if (FD_flag < 3)
			{
				close(sd2);
			}
		}
		else if (p == 0)
		{
			/*TODO*/printf("FD_FLAG: %d\n", FD_flag);
			// new Observer
			if(FD_flag ==1)
			{
				char valid ='N';
				if(num_observer < 255)
				{
					valid='Y';
				}
				
				send(sd2, (void *) &valid, sizeof(char), 0);

				if(valid == 'Y')
				{
					//timer for observer
					time_t begin_loop;
					begin_loop=time(NULL);
					time_t end_loop;
					int time_check=1;

					while(time_check)
					{
						uint8_t recv_length;
        	                                recv(sd2, (void *) &recv_length, sizeof(uint8_t), 0);
                	                        printf("Recived %i", recv_length);
						//recive username
                        	                char new_name[recv_length + 1];
                                	        recv(sd2, (void *) &new_name, recv_length*sizeof(char), 0);
                                        	new_name[recv_length] = '\0';
						printf("Recived %s as %i bytes", new_name, recv_length);
						
						Node *trv = root;
						if(trv == NULL)
                	                        {
                        	                	//There are no participants at all
							valid='N';
                                        	}
	                                        else
        	                                {
							valid='N';
                        	                        while(trv != NULL)
                                	                {
                                        	                if(strcmp(trv->username, new_name)==0)
								{
									valid='T';
									if(trv->observer_sd==-1)
									{
										trv->observer_sd=sd2;
										num_observer++;
										valid='Y';
									}
								}
								trv=trv->next;
							}
                                                }
                
					
						send(sd2, (void *) &valid, sizeof(char), 0);
						if(valid=='Y')//send new observer has joined
						{
							char join_msg[26]="A new observer has joined";

                                                        uint8_t send_length = 25;

                                                        trv=root;
                                                        while(trv!=NULL)
                                                        {
                                                                if(trv->observer_sd != -1)
                                                                {
                                                                        send(trv->observer_sd, (void *)&join_msg, send_length, 0);
                                                                }
                                                                trv=trv->next;
                                                        }
						}
						else if(valid=='T')
						{
							//reset timer ask again
							begin_loop=time(NULL);
						}
						
						//update time check, if valid='N' auto pop out
						end_loop = time(NULL);
						
						if(end_loop-begin_loop>60 || valid=='Y' || valid=='N')
						{
							time_check=0;
						}
					}
					
					if(valid=='N'||valid=='T')
					{
						close(sd2);
					}
				}
				else
				{
					close(sd2);
				}
			}//end of new Observer
		
			//new Participant
			else if( FD_flag == 2)
			{
				char valid = 'N';
				if ((num_FD -2) < 255)
				{
					valid = 'Y';
				}
				
				send(sd2, (void *) &valid, sizeof(char), 0);
				/*TODO*/printf("Server sent valid\n");
				if(valid == 'Y')
				{
					time_t begin_loop;
                                        begin_loop=time(NULL);
                                        time_t end_loop;
                                        int time_check=1;

					while(time_check)
					{
						/*TODO*/printf("made it inside time check Loop\n");
						//recive length of username
						uint8_t recv_length;
						recv(sd2, (void *) &recv_length, sizeof(uint8_t), 0);
						/*TODO*/printf("Recived %i\n", recv_length);
						//recive username
						char new_name[recv_length + 1];
						recv(sd2, (void *) &new_name, recv_length*sizeof(char), 0);
						new_name[recv_length] = '\0';
						/*TODO*/printf("Recived %s as %i bytes\n", new_name, recv_length);
						//check if user name is taken (while loop through linked list)
					

						//character check 
						int i=0;
						int char_Check = 1;
						while(new_name[i]!='\0')
						{
							if(isalnum(new_name[i])==0)
							{
								if(new_name[i]!=95 && new_name[i]!=45)
								{
									char_Check=0;
								}
							}
							i++;
						}

						Node *trv = root;
						int taken = 0;
						while(trv != NULL)
						{
							if (new_name == trv->username)
							{
								taken = 1;
								send(sd2, (void *) &valid, sizeof(char), 0);
							}
							trv = trv->next;
						}
						
						printf("Valid name data ::%i::%i::\n", char_Check, taken);	
						if(char_Check==1 && taken == 0)
						{
							valid='Y';
							/*TODO*/printf("Sending %c\n", valid);
							send(sd2, (void *) &valid, sizeof(char), 0);
							struct Node *new_Node;
							new_Node = malloc(sizeof(struct Node));
	
							//setting new node data
							strcpy(new_Node->username,new_name);
							new_Node->participant_sd=sd2; //TODO passing by reference could be a problem here
							new_Node->observer_sd=-1;
							new_Node->active=1;
							new_Node->next = NULL;
	
							//Put new node on linked list
							trv = root;
							/*TODO*/printf("sent Y then set new_node data\n");
							if(trv == NULL)
							{
								root=new_Node;
							}
							else
							{
								while(trv->next != NULL)
								{
									trv=trv->next;
								}
								trv->next = new_Node;
							}
	
							//add new participant to fd set for select
							FD_SET(new_Node->participant_sd, &participants);
							num_FD++;
							/*TODO*/printf("fd set successful, num_fd == %i\n", num_FD);
							
							if(new_Node->participant_sd>max_FD-1)
							{
								max_FD=new_Node->participant_sd+1;
							}
	
							//inform observers of new participant
							char join_msg[16+strlen(new_name)+1];
	                	                        strcpy(join_msg, "User ");
        		                                strcpy(join_msg, new_name);
        	        	                        strcpy(join_msg, " has joined");
	
        	                	                uint8_t send_length = 16+strlen(new_name);
	
	        	                                trv=root;
        		                                while(trv!=NULL)
	                	                        {
                        	                	        if(trv->observer_sd != -1)//TODO check to make sure its and active participant
                                		                {
                                	        	                send(trv->observer_sd, (void *)&join_msg, send_length, 0);
                        	                        	}
	        	                                        trv=trv->next;
        		                                }
							/*TODO*/printf("Welcome notification sent to all observers\n");
						}
						else if(char_Check==0)
						{
							valid = 'I';
							send(sd2, (void *) &valid, sizeof(char), 0);
						}
						else
						{
							valid = 'T';
							send(sd2, (void *) &valid, sizeof(char), 0);
							begin_loop=time(NULL);
						}
					
						//update time Check
						end_loop = time(NULL);
						if(end_loop-begin_loop>60 || valid=='Y')
                                                {
                                                        /*TODO*/printf("exiting time loop\n");
							time_check=0;
                                                }
					}//end of time_check loop

					//if you time out, act accordingly
					if(valid=='I'||valid=='T')
					{
						/*TODO*/printf("you have timed out\n");
						close(sd2);
					}
				}
				else //Valid ='N'
				{
					close(sd2);	
				}	
			}//end of new Participant
		
			//active Participant
			else if(FD_flag == 3)
			{
				
				//figure out who sd2 should be
        	                Node *trv = root;
				Node *cn = NULL;

                	        while(trv != NULL)
                       		{
	                                if(FD_ISSET(trv->participant_sd, &participants))
        	                        {
                	                        sd2 = trv->participant_sd;
						cn = trv;
                        	        }
	                                trv = trv->next;
        	                }


				//recv length of message
				int dis_check;
				uint16_t message_len;
				
				dis_check = recv(sd2,(void *) &message_len,sizeof(uint16_t),0);
				if(dis_check == 0 || message_len > 1000)
				{
					//disconnect processing
					cn->active = 0;
					cn->observer_sd = -1;
					FD_CLR(sd2, &participants);
					close(sd2);
				}
				else
				{
					char to_send[message_len + 15]; //extra space for formating and null
					char message[message_len + 1];
					int private_flag = 0;
					char catcher[11];
					
					recv(sd2,(void *) &message, message_len, 0);
					
					if(message[0] == '@')
					{
						//this is a private message
						private_flag = 1;
						strcpy(to_send, "%");
						//capture username
						int i =1;
						while(message[i] != ' ') 
						{
							catcher[i-1] = message[i];
							i++; 
						}
						
					}
					else
					{
						strcpy(to_send, ">");
					}

					int spaces = 11-strlen(cn->username);
					for(int i = 0;i < spaces; i++)
					{
						strcpy(to_send, " ");
					}
					strcpy(to_send, cn->username);
					strcpy(to_send, ": ");
					strcpy(to_send, message);
					
					//send to all observers
					if (private_flag == 0)
					{
						trv=root;
						while(trv!=NULL)
						{
                        	                	if(trv->observer_sd != -1)
                                	                {
								uint16_t send_len = strlen(to_send);
								send(trv->observer_sd, (void *) &send_len, sizeof(uint16_t),0);
                                                		send(trv->observer_sd, (void *)&to_send, send_len, 0);
	                                                }
        	                                        trv=trv->next;
                	                        }
					}
					else
					{
						//sending message to only one observer
						trv = root;
						int catch_sd = -1;
						int catcher_ready = 0;
						
						while(trv != NULL)
						{
							if(strcmp(trv-> username, catcher) == 0)
							{
								if(trv->observer_sd != -1)
								{
									if(trv->active == 1)
									{
										catcher_ready = 1;
										catch_sd = trv->observer_sd;
									}
								}	
							}
						}

						if(catcher_ready)
						{
							uint16_t send_len = strlen(to_send);
                                                        send(catch_sd, (void *) &send_len, sizeof(uint16_t),0);
                                                        send(catch_sd, (void *) &to_send, send_len, 0);
						}
						else
						{
							char err_msg[32+strlen(catcher)];
							uint16_t err_len = 31+strlen(catcher);
							strcpy(err_msg, "Warning: user ");
							strcpy(err_msg, catcher);
							strcpy(err_msg, " doesn't exist...");

							send(cn->observer_sd, (void *) &err_len, sizeof(uint16_t),0);
                                                        send(cn->observer_sd, (void *) &err_msg, err_len, 0);
						}
					}
				}
			}//end of active Participant
			/*TODO*/printf("exiting the child\n");
			exit(0);
		}//end of child process 
	}
	exit(EXIT_SUCCESS);
}

