/* time_server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>

#define max_content_name 5

int current_size = 0;

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */

struct addr_pdu{
	char type;
	char username[10];
	char content_name[10];
	struct sockaddr_in addr;
} buf, rbuf;

struct size_pdu{
	char type;
	char username[10];
	char content_name[10];
	int size;
} o_pdu;

struct err_ack_pdu{
	char type;
	char msge[100];
} err,ack,list2;

struct list{
	char username[10];
	char content_name[10];
	struct sockaddr_in addr;
};

struct list content_list[max_content_name];
 
bool peer_content_in_list(struct list list1[max_content_name], char *peer_name, char *content_name);
bool content_in_list(struct list list1[max_content_name], char *content_name);
int get_index(struct list list1[max_content_name], char *content_name);
int removeFromList(struct addr_pdu list1);
 
int main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	//char	buf[100];		/* "input" buffer; any size > 0	*/
	
	char    *pts;
	int	sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
	struct  sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type;        /* socket descriptor and socket type    */
	int 	port=3000;
                                                                                

	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "can't creat socket\n");
                                                                                
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
        listen(s, 5);	
	alen = sizeof(fsin);
	
	int n, i=0, index = 0, ctr = 0, j = 0;
	bool flag = false, deregister_success = false;
		
	while (1) 
	{
		if(recvfrom(s, &buf, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen) < 0)
		{
			fprintf(stderr, "recvfrom error\n");
		}	
		
		switch(buf.type)
		{
			case 'O':
			{
				// list all contents available in index server
				if(current_size > 0)
				{
					// list is not empty
					list2.type = 'O';
					for(i = 0; i < current_size; i++)
					{
						if(i == 0)
						{
							strcat(list2.msge, content_list[i].content_name);
							strcat(list2.msge, "\n");
						}
						else
						{
							int k = 0;
							for(j = 0; j <= i - 1; j++)
							{
								if(strcmp(content_list[i].content_name, content_list[j].content_name) == 0)
								{
									k = 1;
									break;
								}
							}
							if(k == 0)
							{
								strcat(list2.msge, content_list[i].content_name);
								strcat(list2.msge, "\n");
							}
						}
					}
					printf("%s\n", list2.msge);
					sendto(s, &list2, sizeof(list2), 0, (struct sockaddr *)&fsin, sizeof(fsin));
					memset(list2.msge, 0, current_size);
				}
				
				else
				{
					// empty list;
					printf("list empty\n");
					err.type = 'E';
					strcpy(err.msge, "Empty list");
					sendto(s, &err, sizeof(err), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				} 
				break;
			}
			
			case 'R':
			{
				printf("PDU type received from peer: %c\n", buf.type);
				printf("Content registry request from %s\n", buf.username);
				printf("content name: %s\n", buf.content_name);
				printf("peer port #: %u\n", buf.addr.sin_port);
				
				// restricted the content_name_list to 5 
				// check if there is room for content registration
				if(current_size >= max_content_name)
				{
					printf("Index server has reached max limit of content registration\n\n");
					err.type = 'E';
					strcpy(err.msge, "Index server has reached max limit of content registration");
					sendto(s, &err, sizeof(err), 0, (struct sockaddr *)&fsin, sizeof(fsin));
					break;
				}
				
				// need to check if peer with the same name and same content already registered
				flag = peer_content_in_list(content_list, buf.username, buf.content_name);
				
				// if content_name isn't registered and there is still space to hold content names
				if(flag == false && current_size < max_content_name)
				{		
					strcpy(content_list[current_size].username, buf.username);
					strcpy(content_list[current_size].content_name, buf.content_name);
					
					// store the address as well
					content_list[current_size].addr = buf.addr;
					printf("port # in content_name_list[%d]: %u\n", current_size, content_list[current_size].addr.sin_port);
					//printf("buf.content_name: %s\n", buf.content_name);
					printf("content_list[%d].content_name: %s\n", current_size, content_list[current_size].content_name);
					printf("Content server: %s\nContent server address: %d\n\n", content_list[current_size].username, content_list[current_size].addr.sin_addr.s_addr);
					current_size++;
					ack.type = 'A';
					strcpy(ack.msge, "Acknowledged content");
					sendto(s, &ack, sizeof(ack), 0, (struct sockaddr *)&fsin, sizeof(fsin));
					break;
				}
				else
				{
					// need to send an E-type pdu back to the peer
					err.type = 'E';
					strcpy(err.msge, "Choose another name");
					sendto(s, &err, sizeof(err), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}			
				break;
			}
				
			case 'S':
			{
				printf("PDU type received from peer: %c\n", buf.type);
				// check if the content is available for download
				flag = content_in_list(content_list, buf.content_name);
				if(flag == false)
				{
					// content is not available
					err.type = 'E';
					printf("Content is not available\n");
					strcpy(err.msge, "Content is not available");
					sendto(s, &err, sizeof(err), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				else
				{
					// content is available
					printf("Content is available\n");
					rbuf.type = 'S';
					// return index of list of content data
					index = get_index(content_list, buf.content_name);
					if(index != -1)
					{
						rbuf.addr = content_list[index].addr;
					}
					strcpy(rbuf.username, content_list[index].username);
					strcpy(rbuf.content_name, content_list[index].content_name);
					sendto(s, &rbuf, sizeof(rbuf), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				break;
			}
			
			case 'T':
			{
				int success;
				success = removeFromList(buf);
				if(success == 0)
				{
					printf("De-register content successful\n");
					ack.type = 'A';
					strcpy(ack.msge, "De-register content successful");
					sendto(s, &ack, sizeof(ack), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				else
				{
					printf("De-register content unsuccessful\n");
					err.type = 'E';
					strcpy(err.msge, "De-register content unsuccessful\nYou did not register anything\n");
					sendto(s, &err, sizeof(err), 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				break;
			}
			
			default:
			{
				printf("defaulted\n\n");
				break;		
			}
		}
	}
	printf("Terminating now..\n");
}

int removeFromList(struct addr_pdu list1)
{
	int i = 0, n = current_size;
	for(i = 0; i < n; i++)
	{
		if(strcmp(content_list[i].username, list1.username) == 0 && strcmp(content_list[i].content_name, list1.content_name) == 0)
		{
			printf("PDU to remove: \nPeer Name: %s\nContent Name: %s\n", content_list[i].username, content_list[i].content_name);
			for(i; i < current_size; i++)
			{
				content_list[i] = content_list[i+1];
			}
			current_size--;
			return 0;
		}
	}
	return -1;
}

// this function returns true if the peer name is already in the list
bool peer_content_in_list(struct list list1[max_content_name], char *peer_name, char *content_name)
{
	int i = 0;
	//printf("size: %d\n", size);
	// loop through and check if the peer name already exists
	for(i = 0; i < max_content_name; i++)
	{
		if(strcmp(list1[i].username, peer_name) == 0 && strcmp(list1[i].content_name, content_name) == 0)
		{
			// meaning the peer_name already exists
			// can return true
			printf("Peer: %s\nAlready registered content: %s\n", list1[i].username, list1[i].content_name);
			printf("Need to choose another name..\n");
			return true;
		}
	}
	return false;
}

// this func returns true if the content is available, and it loops such that it retrieves the most recently stored
bool content_in_list(struct list list1[max_content_name], char *content_name)
{
	int i = 0;
	//printf("size: %d\n", size);
	// loop through and check if the content_name already exists
	for(i = max_content_name - 1; i >= 0; i--)
	{
		if(strcmp(list1[i].content_name, content_name) == 0)
		{
			// meaning content is available
			// can return true
			printf("Content: %s is available for download\n", list1[i].content_name);
			return true;
		}
	}
	return false;
}

// might need to fix something here if multiple peers upload same content name
int get_index(struct list list1[max_content_name], char *content_name)
{
	int i = 0;
	for(i = max_content_name; i >= 0; i--)
	{

		if(strcmp(list1[i].content_name, content_name) == 0)
		{
			return i;
		}

	}
	return -1;
}

