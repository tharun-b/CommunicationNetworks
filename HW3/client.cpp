/*
 ** client.c -- ./client <proxy addr> <proxy port> <url to retrieve>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>

using namespace std;

#define MAXRETSIZE 70000

int recvtimeout(int s, char *buf, int len, int timeout)
{
	fd_set fds;
	int n;
	struct timeval tv;

	// set up the file descriptor set
	FD_ZERO(&fds);
	FD_SET(s, &fds);

	// set up the struct timeval for the timeout
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	// wait until timeout or data received
	n = select(s+1, &fds, NULL, NULL, &tv);
	if (n == 0) return -2; // timeout!
	if (n == -1) return -1; // error

	// data must be here, so do a normal recv()
	return recv(s, buf, len, 0);
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char * defURL = NULL;

	if(strstr(argv[3], "ttp://") != NULL){
		defURL = (char *)calloc(strlen(argv[3]),1);
		strcpy(defURL, argv[3]);
		cout << endl << defURL<<endl;
	}else{
		defURL = (char *)calloc(7 + strlen(argv[3]),1);
		strcpy(defURL, "http://");
		strcat(defURL, argv[3]);
	}
	if (argc != 4) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	send(sockfd, defURL, strlen(defURL), 0);

	char RETbuf[MAXRETSIZE+1];
	char filename[500];
	int file_flag = 0;
	int recvn = 0;
	FILE *txtfile;

	int r_flag = 1;
	while(r_flag)
	{
		(recvn = recvtimeout(sockfd, RETbuf, MAXRETSIZE, 2));
		cout << recvn << "<-- size recieved: " <<endl;
		if(recvn<=0){
			r_flag = 0;
			cout<<"Finished a transmission"<<endl;
			break;
		}

		if (strstr(RETbuf, "404 Not Found")){
			cout << "404 Not Found!\n";
			break;
		}

		if (!file_flag){//first time
//			struct stat st = {0};

//			if (stat("clntfolder", &st) == -1) {
//				mkdir("clntfolder", 0700);
//			}
			cout<<RETbuf<<endl;

			file_flag =1;
//			strcpy(filename, "/");
			//RETbuf[recvn]='\0';
			strcat(filename, strrchr(defURL, '/')+1);
			if (strstr(defURL, ".txt")){
				txtfile = fopen(filename, "w");
				if (txtfile == NULL){
					fprintf(stderr, "Can't open output file %s!\n",
							filename);
					exit(1);
				}
				char *ptr = strstr(RETbuf, "\r\n\r\n") ;
				if(ptr){
					fputs(ptr+4,txtfile);
				}else{
					cout<<"No message part!?"<<endl;
				}
			}else if(strstr(defURL, ".jpg")){
				txtfile = fopen(filename, "wb");
				if (txtfile == NULL){
					fprintf(stderr, "Can't open output file %s!\n",
							filename);
					exit(1);
				}
				fwrite(strstr(RETbuf, "\r\n\r\n")+4, 1, recvn - (strstr(RETbuf, "\r\n\r\n") + 4 - RETbuf), txtfile);
			}
		}else{
			if(strstr(filename, ".txt")){
				fputs(RETbuf,txtfile);
			}else if(strstr(filename, ".jpg")){
				fwrite(RETbuf, 1, recvn, txtfile);
			}
		}
		memset(RETbuf,0,MAXRETSIZE);
	}
	cout<<"FILE closed"<<endl;
	if (recvn == 0){
		fclose(txtfile);
	}

	free(defURL);
	close(sockfd);

	return 0;
}
