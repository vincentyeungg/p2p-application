/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
                                                                                
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>

#define	BUFSIZE 1460

#define	MSG		"Any Message \n"


/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
 
 struct addr_pdu{
 	char type;
 	char peer_name[10];
 	char content_name[10];
 	struct sockaddr_in addr;
 } spdu, rpdu;
 
 struct content_download_pdu{
 	char type;
 	char peer_name[10];
 	char content_name[10];
 } apdu, bpdu;
 
 struct c_pdu{
 	char type;
 	char data[BUFSIZE];
 };
 
 struct data_pdu{
 	char type;
 	char data[100];
 } dpdu, o_pdu;
 
 // function to get obtain peer addr for content registration
 void get_addr(struct addr_pdu *pdu, int tcp_s);
 // function to read data from content_server for download
 int download_file_tcp(int tcp_s, const char *filename);
 // function to send data to content client for download
 int send_file_tcp(int tcp_s);
 
int main(int argc, char **argv)
{
	char	*host = "localhost";
	int	port = 3000, alen;
	char	now[100];		/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin, client, server;	/* an Internet endpoint address		*/
	int	s, n, type, tcp_s;	/* socket descriptor and socket type	*/
	fd_set afds, rfds;
	
	char input;
	int i=0, tcp_s_temp1, tcp_s_temp2;

	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Can't create UDP socket \n");
	
	
	/* Create a stream socket	*/	
	if ((tcp_s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't create a tcp socket\n");
	}
                                                                                
    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to server");
	
	/* project peer code starts here */
	
	printf("Welcome\n");
	
	// each peer should have their own name that is unique
	printf("Enter username here: \n");
	scanf(" %s", spdu.peer_name);
	spdu.peer_name[9] = '\0';
	printf("'?' to display available options\n");
	printf("Command: \n");
	
	do
	{
		// initialize current set
		FD_ZERO(&afds);
		FD_SET(tcp_s,&afds);
		FD_SET(tcp_s_temp1,&afds);
		FD_SET(0,&afds);
		
		memcpy(&rfds,&afds,sizeof(rfds));
		
		if(select(FD_SETSIZE,&rfds,NULL,NULL,NULL) < 0)
		{
			printf("Select error\n");
		}
		
		if(FD_ISSET(0,&rfds))
		{
			printf("\n");
			scanf(" %c", &input);
			
			switch(input)
			{
				case '?':
				{
					// list the commands available
					printf("R-Content Registration\n");
					printf("T-Content De-Registration\n");
					printf("L-List Local Content\n");
					printf("D-Download Content\n");
					printf("O-List all the On-Line Content\n");
					printf("Q-Quit\n\n");
					printf("Enter command: \n");
					break;
				}
					
				case 'O':
				{
					// list all registered content available in index server
					o_pdu.type = 'O';
					printf("Requesting list of content available in index server..\n");
					strcpy(o_pdu.data, "Requesting list of content");
					write(s, &o_pdu, sizeof(o_pdu));
					printf("Receiving list of contents from index server..\n");
					while((i = read(s, &dpdu, sizeof(dpdu))) > 0)
					{
						// check if server sends error or ack
						if(dpdu.type == 'E')
						{
							printf("List is empty\n\n");
							break;
						}
						if(dpdu.type == 'O')
						{
							printf("%s\n", dpdu.data);
							break;
						}
						break;
					}
					printf("Command: \n");
					break;
				}
				
				case 'L':
				{
					// list contents found in directory
					DIR *d;
					struct dirent *dir;
					d = opendir(".");
					if(d != NULL)
					{
						printf("Listing directory content: \n");
						while((dir = readdir(d)) != NULL)
						{
							printf("%s\n", dir->d_name);
						}
						closedir(d);
					}
					else
					{
						printf("Directory cannot be opened\n");	
					}
					printf("\n");
					printf("Command: \n");
					break;
				}
								
				case 'R':
				{			
					FILE *fp = NULL;
					// setup initial address
					if ((tcp_s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
						fprintf(stderr, "Can't create a tcp socket\n");
					}
					get_addr(&spdu, tcp_s);	
					spdu.type = 'R';
					printf("Enter content name you wish to register: \n");
					scanf(" %s", spdu.content_name);
					spdu.content_name[9] = '\0';
					fp = fopen(spdu.content_name, "r");
					if(fp != NULL)
					{
						write(s, &spdu, sizeof(spdu));
						while((i = read(s, &dpdu, sizeof(dpdu))) > 0)
						{
							// check if server sends error or ack
							printf("Receiving from index server..\n");
							if(dpdu.type == 'A')
							{
								printf("%s\n\n", dpdu.data);
								break;
							}
							else if(dpdu.type == 'E')
							{
								printf("%s\n\n", dpdu.data);
								break;
							}
						}
					}
					else
					{
						printf("File does not exist\nFile not registered\n\n");
					}
					printf("Command: \n");
					break;
				}
					
				case 'D':
				{
					// content download
					apdu.type = 'S';
					printf("Enter content name you want to download: \n");
					scanf(" %s", apdu.content_name);
					apdu.content_name[strlen(apdu.content_name)] = '\0';
					write(s, &apdu, sizeof(apdu));
					while((i = read(s, &rpdu, sizeof(rpdu))) > 0)
					{
						// check if server sends error or s-type
						printf("Receiving from index server..\n");
						if(rpdu.type == 'E')
						{
							printf("Content not available\n\n");
							printf("Command: \n");
							break;
						}
						else if(rpdu.type == 'S')
						{
							printf("Content available\n");
							printf("From:\npeer: %s\ncontent data: %s\n", rpdu.peer_name, rpdu.content_name);
							printf("Port #: %u\n\n", rpdu.addr.sin_port);	
							// setup tcp connection with content server
							if((tcp_s_temp2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
							{
								fprintf(stderr, "Can't create TCP socket.\n");
								break;
							}
							if(connect(tcp_s_temp2, (struct sockaddr*)&rpdu.addr, sizeof(rpdu.addr)) < 0)
							{
								printf("Failed to connect to content server\n");
								break;
							}
							printf("Connect to address: %d\n", rpdu.addr.sin_addr.s_addr);
							
							// retrieve file from addr specified
							int success = download_file_tcp(tcp_s_temp2, rpdu.content_name);
							if(success != 1)
							{
								printf("File download unsuccessful\n");
								close(tcp_s_temp2);
								break;
							}
							printf("File download successful\n");
							close(tcp_s_temp2);
							
							// register new content to this port	
							if ((tcp_s_temp1 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
								fprintf(stderr, "Can't create a tcp socket\n");
								break;
							}
							get_addr(&spdu, tcp_s_temp1);
							spdu.type = 'R';
							printf("New port #: %u\n", spdu.addr.sin_port);
							strcpy(spdu.content_name, rpdu.content_name);
							write(s, &spdu, sizeof(spdu));
							while((i = read(s, &dpdu, sizeof(dpdu))) > 0)
							{
								// check if server sends error or ack
								printf("Receiving from index server..\n");
								if(dpdu.type == 'A')
								{
									printf("%s\n\n", dpdu.data);
									break;
								}
								else if(dpdu.type == 'E')
								{
									printf("%s\n\n", dpdu.data);
									break;
								}
							}
						}
						break;
					}
					printf("Command: \n");
					break;
				}
				
				case 'Q':
				{
					printf("Need to de-register all contents\n"); 
					spdu.type = 'T';
					write(s, &spdu, sizeof(spdu));
					while((i = read(s, &dpdu, sizeof(dpdu))) > 0)
					{
						// check if server sends error or s-type
						printf("Receiving de-register confirmation from index server\n");
						if(dpdu.type == 'E')
						{
							printf("%s\n", dpdu.data);
							break;
						}
						else
						{
							printf("%s\n", dpdu.data);	
							break;	
						}
					}
				
					printf("Exiting program\n");
					// close udp socket
					close(tcp_s);
					close(tcp_s_temp1);
					close(s);
					break;
				}
				
				case 'T':
				{
					// need to re-register content from this peer
					printf("Enter file you want to de-register: \n");
					scanf(" %s", spdu.content_name);
					spdu.content_name[9] = '\0';
					
					printf("Attempting to de-register all contents\n");
					spdu.type = 'T';
					
					write(s, &spdu, sizeof(spdu));
					while((i = read(s, &dpdu, sizeof(dpdu))) > 0)
					{
						// check if server sends error or s-type
						printf("Receiving de-register confirmation from index server\n");
						if(dpdu.type == 'E')
						{
							printf("%s\n", dpdu.data);
							printf("Command: \n");
							break;
						}
						else
						{
							printf("%s\n", dpdu.data);	
							printf("Command: \n");
							break;
						}
					}
					break;
				}
				
				default:
				{
					printf("Invalid Command\n\n");
					printf("Command: \n");
					break;
				}
			}
		}
		
		if(FD_ISSET(tcp_s,&rfds))
		{
			int new_sd;
			// new connection here
			int c_len = sizeof(client);
			new_sd = accept(tcp_s, (struct sockaddr *)&client, &c_len);
	  		if(new_sd > 0)
	  		{
	  			// register new file with content name on index server
			  	int downloaded = 0;
			  	downloaded = send_file_tcp(new_sd);
			  	if(downloaded == 1)
			  	{
			  		printf("File download successful\n\n");
			  		printf("Command:\n");
			  	}
			  	else
			  	{
			  		printf("File download unsuccessful\n\n");
			  		printf("Command:\n");
			  	}
	  		}
	  		close(new_sd);
		}
		
		if(FD_ISSET(tcp_s_temp1,&rfds))
		{
			int new_sd;
			// new connection here
			//printf("Accept new connection\n");
			int c_len = sizeof(client);
			new_sd = accept(tcp_s_temp1, (struct sockaddr *)&client, &c_len);
	  		if(new_sd > 0)
	  		{
	  			// register new file with content name on index server
			  	int downloaded = 0;
			  	downloaded = send_file_tcp(new_sd);
			  	if(downloaded == 1)
			  	{
			  		printf("File download successful\n\n");
			  		printf("Command:\n");
			  	}
			  	else
			  	{
			  		printf("File download unsuccessful\n\n");
			  		printf("Command:\n");
			  	}
	  		}
	  		close(new_sd);
		}
	}
	while(input != 'Q');

	exit(0);
}

int download_file_tcp(int temp_tcp_s, const char *filename)
{
	//request file transfer
	FILE *fp = NULL;
	int n = 0, i = 0;
	struct c_pdu pdu;
	
	apdu.type = 'D';
	strcpy(apdu.peer_name, spdu.peer_name);
	strcpy(apdu.content_name, filename);
	
	write(temp_tcp_s, &apdu, sizeof(apdu));
	
	int j = 0;
	
	while (i = read(temp_tcp_s, &pdu, sizeof(pdu)) > 0)
	{
		if(pdu.type == 'C')
		{
			if(j == 0)
			{
				fp = fopen(filename, "w");
			}
			else if(j > 0)
			{
				fp = fopen(filename, "a");
			}
			if(fp == NULL)
			{
				// error in opening file
				perror("Error");
				printf("Unable to open file\n");
				return -1;
			}		
			printf("Bytes received: %ld\n", strlen(pdu.data));
			fwrite(pdu.data, strlen(pdu.data), 1, fp);
			fclose(fp);
			j++;
		}
		else
		{
			// error pdu
			perror("Error");
			printf("%s\n", pdu.data);
			return -1;
		}
	}
	return 1;
}

int send_file_tcp(int temp_tcp_s)
{
	int ctr = 0;
	FILE *fp = NULL;
	struct c_pdu pdu;
	
	read(temp_tcp_s, &bpdu, sizeof(bpdu));
	
	if(bpdu.type == 'D')
	{
		fp = fopen(bpdu.content_name, "r");
		if(fp == NULL)
		{
			printf("Unable to find file\n");
			strcpy(pdu.data, "File does not exist.");
			pdu.type = 'E';
			write(temp_tcp_s, &pdu, sizeof(pdu));
			perror("Error");
			return -1;
		}
		
		else
		{
			char ch;
			ctr = 0;
			pdu.type = 'C';
			
			while((ch = fgetc(fp)) != EOF)
			{
				pdu.data[ctr++] = ch;
				if((ctr % BUFSIZE) == 0)
				{
					printf("Packet size: %ld\n", strlen(pdu.data));
					write(temp_tcp_s, &pdu, sizeof(pdu));
					printf("Packet sent\n");
					memset(pdu.data, '\0', ctr);
					ctr = 0;
				}
			}
			if(ctr != 0)
			{
			// there are remainder data to send
				printf("Packet size: %d\n", ctr);
				write(temp_tcp_s, &pdu, sizeof(pdu));
				printf("Packet sent\n");
			}
			fclose(fp);
			return 1;
		}
	}
} 

void get_addr(struct addr_pdu *pdu, int tcp_s)
{
	int alen;
	struct sockaddr_in reg_addr;
	
	bzero((char*)&reg_addr, sizeof(struct sockaddr_in));
	reg_addr.sin_family = AF_INET;
	reg_addr.sin_port = htons(0); // random port
	reg_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // local host ip addr
	
	if(bind(tcp_s,(struct sockaddr *)&reg_addr,sizeof(reg_addr)) == -1)
	{
		fprintf(stderr,"Can't bind name to socket\n");
		exit(1);
	}
	
	listen(tcp_s, 1);
	alen = sizeof(struct sockaddr_in);
	getsockname(tcp_s, (struct sockaddr *) &reg_addr, &alen);
	
	printf("TCP socket binded to port #: %u\n", reg_addr.sin_port); //testing
	printf("Address: %d\n", reg_addr.sin_addr.s_addr);
	
	pdu->addr = reg_addr;
	//printf("Port #: %u\n", pdu->addr.sin_port); //testing
}
