#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <rpc/xdr.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFSIZE 516

struct packet
{
	uint16_t blockid; //2 bytes
	char data[512]; // N bytes
}; 


struct client_data
{
	uint16_t socket;
	FILE* file;
	struct sockaddr_in remoteaddr;
   char filename[50];
};

int createSocket(uint16_t Port);
void* processRequest(void *ptr);
char* createError(uint16_t, char* );
char *serverIP;
	
int main(int argc, char **argv) 
{

   unsigned short int serverPort;
  
   if (argc != 3) 
   {
       perror("\nError :\n Usage : ./server server_ip server_port \n");
       exit(1);
   }

   serverIP = argv[1];
   serverPort = atoi(argv[2]);

   printf("serverIP %s serverPort: %d", serverIP, serverPort );
   fflush(stdout);


	char buf[BUFSIZE];
	int recvlen; /* # bytes received */ 
	
	struct sockaddr_in remoteaddr; /* remote address */ 
	socklen_t addrlen = sizeof(remoteaddr); /* length of addresses */ 

   pthread_t new_req;	

   int fd = createSocket(serverPort);
   while(1)
	{
	
   	/* now loop, receiving data and printing what we received */ 
	   printf("\n\nwaiting on port %d\n", serverPort);
	   bzero(buf, BUFSIZE); 
	   recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remoteaddr, &addrlen); 

	   if (recvlen > 4) 
	   {

		   uint16_t recv_opcode = ntohs(*((uint16_t *)buf));
		
		   if (recv_opcode == 1)	//RRQ request
		   {
			   buf[recvlen] = 0;
		
			   printf("\n received file request for: \"%s\"\n", &buf[2]);
			   printf("Sending %s to the Server... \n", &buf[2]);
			   FILE *fs = fopen(&buf[2], "rb");
            char *errorbuf;
			   if(fs == NULL)
			   {
				   //File not found
				   printf("ERROR: File %s not found.\n", &buf[2]);
				   uint16_t length = 2*sizeof(uint16_t) + strlen(&buf[2]) + 1;
               char *ptr = &buf[2];
				   errorbuf = createError(1, ptr);
				   sendto(fd, errorbuf, length, 0, (struct sockaddr *)&remoteaddr, addrlen);
				   free(errorbuf);
               continue;		
			   } 
		
		   	struct stat st;
		   	stat(&buf[2], &st);
		   	if(st.st_size > (65534 * 512 + 511))
		   	{
		   		//File size exceeded
		   		printf("\nError: File size exceeds the maximum allowable size");
               char ptr[] = "Maximum size reached";
		   		errorbuf = createError(3, ptr);
		   		uint16_t length = 2*sizeof(uint16_t) + strlen(ptr) + 1; 
	   			sendto(fd, errorbuf, length, 0, (struct sockaddr *)&remoteaddr, addrlen);
	   			free(errorbuf);
               continue;			
	   		}
		
	   		int newsocket = createSocket(0);
   
	         struct client_data *client_data_ptr;	
		   	client_data_ptr = (struct client_data *)malloc(sizeof(struct client_data));
		   	client_data_ptr->socket = newsocket;
		   	client_data_ptr->file = fs;
		   	client_data_ptr->remoteaddr = remoteaddr;
            memcpy(client_data_ptr->filename, &buf[2], strlen(&buf[2]) + 1);
 
		   	//pthread_t *new_req;
		   	//new_req = (pthread_t *)malloc(sizeof(pthread_t));

	         int checkThread;	
		   	if((checkThread = pthread_create(&new_req, NULL, processRequest, (void *)client_data_ptr)) != 0)
		   	{
		   		
               printf("\nError creating a thread:%d",checkThread);
		   		exit(-1);
		   	}
   
	      }
      }
	}  
}

int createSocket(uint16_t Port)
{

	struct sockaddr_in myaddr; /* our address */ 
	int fd; /* our socket */  	

  	 /* create a UDP socket */ 
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		perror("cannot create socket\n"); 
		return 0; 
  	} 
	
   	/* bind the socket to any valid IP address and a specific port */ 
	memset((char *)&myaddr, 0, sizeof(myaddr)); 
	myaddr.sin_family = AF_INET; 
   inet_pton(AF_INET, serverIP, &(myaddr.sin_addr));

   //Initializing seed for random number
   time_t t;
   srand((unsigned) time(&t));
	
	if(Port != 0)
	{
		myaddr.sin_port = htons(Port); 
		if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
		{ 
      			perror("bind failed at the outset"); 
      			exit(0); 
		} 
	}
	else
	{	
		int count = 0;
		while(++count < 4)
		{
			Port = 1025 + rand() % 64511;
			printf("\nPort created is :%u", Port);
			fflush(stdout);
         myaddr.sin_port = htons(Port); 	
			if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
			{ 
      		if(count < 4)
					continue;
				else
				{
					printf("\nbind failed"); 
      			exit(0); 
				}		
			}
         else
            break;
		}  
      				 
	} 
	
	return fd;
}


char* createError(uint16_t errcode, char* errormsg)
{
//   printf(" Size of error message %ld",strlen(errormsg));
   int length = strlen(errormsg) + sizeof(uint16_t) * 2 + 1 ;
   char *buffer = (char *)malloc(length);
   char *pos = buffer;
   bzero(pos, length);

   uint16_t opcode = htons(5);
   memcpy(pos, &opcode, sizeof(uint16_t));
   pos += sizeof(opcode);
   errcode = htons(errcode);
   memcpy( pos, &errcode, sizeof(uint16_t));
   pos += sizeof(errcode);

   memcpy( pos, errormsg, strlen(errormsg) + 1);
   return buffer;

}
   
   
//HandleClient code
void* processRequest(void *voidptr)
{
	
   struct client_data *ptr = (struct client_data *) voidptr;

   struct sockaddr_in remoteaddr = ptr->remoteaddr; /* remote address */
   socklen_t addrlen = sizeof(remoteaddr);
	
	int packetlen;
	char *pos;
	char *inputpos;
	char outputbuf[512+2+2];
	char inputbuf[2+2+512];		//Assuming error message cant be more than 512 bytes(including null)
	unsigned short int op, block;
	unsigned short int opcode, blockid = 1;

	size_t bytes;
	int flag = 0;
   int retransmitCount = 0;

	struct timeval tv;

	tv.tv_sec = 3;  /* 3 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	if (setsockopt(ptr->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
	{
		printf("\nError in setting timeout value");
	}


	while(1)
	{
		if(flag == 0)	//send new data
		{
			//Read data from file
			bytes=fread(&outputbuf[4], sizeof(char), 512, ptr->file);
			packetlen = sizeof( opcode ) + sizeof( blockid ) + bytes;
			//buf1 = (char *)malloc(packetlen); 
			pos = outputbuf;
		
			//pack data in buffer
			op=htons(3);
			block=htons(blockid);
			memcpy( pos, &op, sizeof(op) );
			pos += sizeof(op);
			memcpy( pos, &block, sizeof(block) );
			pos += sizeof(block);
			printf("\nSent:%u", block);

			if(sendto(ptr->socket, outputbuf, packetlen, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) 
			{
				fprintf(stderr, "ERROR: Failed to send file %s.\n", ptr->filename);
				flag = -1;
				break;
			}

			flag = 1;	  //indicates data send waiting for ack
         retransmitCount = 0;

			if(bytes < 512)
			{
			 	flag = 2; //indicates last packet
	      }

		}
	
		//Read the client response		
		if((recvfrom(ptr->socket, inputbuf, BUFSIZE, 0, (struct sockaddr *)&remoteaddr, &addrlen)) >= (long)(2*sizeof(uint16_t)))	//Implies timeout
		{		
			
   		uint16_t recv_opcode = ntohs(*((uint16_t *)inputbuf));
			inputpos = inputbuf + sizeof(uint16_t);

			uint16_t recv_blockid;
         uint16_t errorcode;

			switch (recv_opcode)
			{

				case 4: 
					recv_blockid = ntohs(*((uint16_t *)inputpos));
					if(recv_blockid == blockid)
					{	
						printf("\tACK %u",recv_blockid);
                  if(flag != 2)		//not last block
						{
							flag = 0;
							++blockid;
						}
					}
					break;
				
				case 5:	 		
					errorcode = ntohs(*((uint16_t *)inputpos));
					inputpos += sizeof(uint16_t);
					printf("\n\nError::");				
					switch (errorcode)
					{
	   				case 1: printf("File not found.");
		   				break;
			   		case 2: printf("Access violation.");
				   		break;
				   	case 3: printf("Disk full or allocation exceeded.");
				   		break;
				   	case 4: printf("Illegal TFTP operation.");
				   		break;
				   	case 5: printf("Unknown transfer ID.");
				   		break;
				   	case 6: printf("File already exists.");
				   		break;
				   	case 7: printf("No such user");
				   		break;
				   	default:
				   		break;
					}
					printf(" %s",inputpos);
					flag = -2;		//Indicates error message
					break;

				default:
					break;

         }
		
			if(flag != 0)	// implies last block or error condition
				break;
		}
      else
      {
         if(retransmitCount++ > 3)  //Retransmit three times 
            break;

		   //Retransmitting
		   if(sendto(ptr->socket, outputbuf, packetlen, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) 
		   {
			   fprintf(stderr, "ERROR: Failed to send file %s.\n", ptr->filename);
		   	break;
  		   }  		
				
	   }
   }
	
   if(flag == 1)
		printf("\n File %s from server was Sent!\n", ptr->filename);
	close(ptr->socket);
	fclose(ptr->file);
	free(ptr);
	pthread_exit((void*) ptr);
}  
