/*
 ** proxy.cpp
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <unistd.h>
#include "cache.h"
#include <sys/stat.h>
#include <time.h>
using namespace std;

#define MAXDATASIZE 520
#define MAXRETSIZE 70000
#define MAXENTRY 10 // max entries in cache
#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int send_from_cache(char *cachepath,
		char *buf,
		int sockid
		)
{
	//---------Cache Hit!------------
	// the mycache.get will automatically refresh the cache
	// send Reponse to client
	//mycache.PrintCache();
	cout << "Cache Hit: " << cachepath << endl;
	FILE *rfile;
	char* rbuf;
	int sent;
	if(strstr(buf, ".txt")){
		rbuf = (char *) calloc(MAXRETSIZE, 1);
		strcpy(rbuf, "\r\n\r\n");
		char ch;
		rfile = fopen(cachepath, "r");
		if (rfile == NULL) {
			fprintf(stderr, "Can't open output file %s!\n",
					cachepath);
			exit(1);
		}
		while((ch = fgetc(rfile)) != EOF){
			//cout << "ping : " << ch<<endl ;
			strncat(rbuf, &ch, 1);
		}
		fclose(rfile);
		sent = send(sockid, rbuf, strlen(rbuf), 0);
	}else if(strstr(buf, ".jpg")){
		rfile = fopen(cachepath, "rb");

		// obtain file size:
		long lsize;
		fseek(rfile, 0, SEEK_END);
		lsize = ftell(rfile);
		rewind(rfile);

		rbuf = (char *) calloc(lsize+4, 1);
		strcpy(rbuf, "\r\n\r\n");
		fread(rbuf+4, 1, lsize, rfile);
		fclose(rfile);
		sent = send(sockid, rbuf, lsize+4, 0);
	}
//	cout<< ">>"<<sent<<"\t";
free(rbuf);
memset(buf, 0, MAXDATASIZE);
return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// compare "exptime" with current time, 1-exptime has been passed, 0-not passed
int timecmp(char * exptime){

	//current time
	time_t rawtime;
	struct tm * ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	cout << ptm->tm_wday << ", " << ptm->tm_mday << " " << ptm->tm_mon
			<< " " << ptm->tm_year << " " << ptm->tm_hour << ":"
			<< ptm->tm_min << ":" << ptm->tm_sec << " " << endl;

//	cout << exptime <<endl;
	//char * wday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	//char * month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char * wdays;
	wdays = (char*) calloc(21, 1);
	strcpy(wdays, "SunMonTueWedThuFriSat");
	char * months;
	months = (char*) calloc(36, 1);
	strcpy(months, "JanFebMarAprMayJunJulAugSepOctNovDec");

	// check time zone
	if (!strstr(exptime, "GMT")){
		cout << "Not Greenwich Mean Time!\n";
		return -1;
	}

	struct tm * exptm;
	exptm = (struct tm *)calloc(1, sizeof(struct tm));
	char *tmptime;
	tmptime = (char *)calloc(4, 1);

	// days since Sunday: 0-6
	strncpy(tmptime+1, exptime, 3);
	exptm->tm_wday = (strstr(wdays, tmptime+1) - wdays)/3.0;

	// day of the month: 1-31
	strncpy(tmptime+2, strstr(exptime, ",") + 2, 2);
	exptm->tm_mday = atoi(tmptime+2);

	// months since January: 0-11
	strncpy(tmptime+1, strstr(exptime, ",") + 5, 3);
	exptm->tm_mon = (strstr(months, tmptime+1) - months)/3.0;

	// years since 1900
	strncpy(tmptime, strstr(exptime, ",") + 9, 4);
	exptm->tm_year = atoi(tmptime) - 1900;

	// hours since midnight: 0-23
	strncpy(tmptime+2, strstr(exptime, ":") - 2, 2);
	exptm->tm_hour = atoi(tmptime+2);

	// minutes after the hour: 0-59
	strncpy(tmptime+2, strstr(exptime, ":") + 1, 2);
	exptm->tm_min = atoi(tmptime+2);

	// seconds after the minute: 0-60
	strncpy(tmptime+2, strstr(exptime, ":") + 4, 2);
	exptm->tm_sec = atoi(tmptime+2);

//	 expire time in cache
	cout << exptm->tm_wday << ", " << exptm->tm_mday << " " << exptm->tm_mon
			<< " " << exptm->tm_year<< " " << exptm->tm_hour << ":"
			<< exptm->tm_min << ":" << exptm->tm_sec << " " << endl;

	free(tmptime);
	free(wdays);
	free(months);

	if (exptm->tm_year > ptm->tm_year)
		return 0;
	else if((exptm->tm_year < ptm->tm_year))
		return 1;
	else
		if(exptm->tm_mon > ptm->tm_mon)
			return 0;
		else if(exptm->tm_mon < ptm->tm_mon)
			return 1;
		else
			if(exptm->tm_mday > ptm->tm_mday)
				return 0;
			else if(exptm->tm_mday < ptm->tm_mday)
				return 1;
			else
				if(exptm->tm_hour > ptm->tm_hour)
					return 0;
				else if(exptm->tm_hour < ptm->tm_hour)
					return 1;
				else
					if(exptm->tm_min > ptm->tm_min)
						return 0;
					else if(exptm->tm_min < ptm->tm_min)
						return 1;
					else
						if(exptm->tm_sec > ptm->tm_sec)
							return 0;// not expired
						else return 1;
	return 1;  //expired
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		cout << "Input parameters are not equal to 4." <<endl;
		exit(1);
	}

	struct addrinfo hints, *servinfo, *p;
	//    socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	int rv;
	char *pivot;

	int web_sockfd;
	int listener_sockfd;// listening socket descriptor
	int client_sockfd;   // New client sockfd
	struct sockaddr_storage their_addr; // client address

	fd_set master;    // master file descriptor list
	fd_set temp_fds_list;  // temp file descriptor list for select()

	socklen_t addrlen;
	int fdmax;        // maximum sockfd number
	int i;

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&temp_fds_list);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next){
		if ((listener_sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1)	{
			perror("server: socket");
			continue;
		}

		if (setsockopt(listener_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1)	{
			perror("setsockopt");
			exit(1);
		}

		if (bind(listener_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(listener_sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(listener_sockfd, BACKLOG) == -1)	{
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	// add the listener to the master set
	FD_SET(listener_sockfd, &master);

	// keep track of the biggest file descriptor
	fdmax = listener_sockfd; // so far, it's this one

	Cache mycache(MAXENTRY);
	char buf[MAXDATASIZE];
	char GETbuf[MAXDATASIZE];
	char RETbuf[MAXRETSIZE+1];
	char ip4[15];
	char path[MAXPATHSIZE];
	char exp[MAXTIMESIZE];
	FILE *txtfile;
	int expflag = 0;
	/* loop for managing clients */
	while(1)
	{
		temp_fds_list = master; // copy it
		if (select(fdmax+1, &temp_fds_list, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &temp_fds_list)) { // we got one!!
				if (i == listener_sockfd) {
					// handle new connections
					addrlen = sizeof their_addr;
					client_sockfd = accept(listener_sockfd,
							(struct sockaddr *)&their_addr,
							&addrlen);
					if (client_sockfd == -1) {
						perror("accept");
					} else {
						FD_SET(client_sockfd, &master); // add to master set
						if (client_sockfd > fdmax) {    // keep track of the max
							fdmax = client_sockfd;
						}
					}
				}
				else
				{
					int recvn =0 ;
					memset(buf,0,MAXDATASIZE);
					recvn = recv(i, buf, MAXDATASIZE, 0);

					// handle data from a client
					if (recvn <= 0) {
						// got error or connection closed by client
						if (recvn == 0) {
							// connection closed
							cout << "client disconnected."<<endl;
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
						break;
					}
//					cout << "recvn: " << recvn <<endl;
					cout << "URL requested by client: " << buf << endl;

					string URLstr(buf);
					char* cachepath;
					cachepath = (char*) calloc(MAXPATHSIZE, 1);
					mycache.PrintCache();
					cachepath = mycache.get(URLstr);

					// check expire or not
					char * cacheexp;
//					cout << "ping\n";
					cacheexp = (char*) calloc(MAXTIMESIZE, 1);

//					cout << "ping\n";
					cacheexp = mycache.getexpire(URLstr);

					if (cachepath != NULL)/* only when file is in the cache */
					{
						cout << "cacheexp--> "<<cacheexp<<endl;

						expflag = timecmp(cacheexp); // 1- expire, 0- not yet
						cout << "expflag--> "<<expflag<<endl;
					}
					if(cachepath != NULL && expflag == 0)/* only file is not expired */
					{
						send_from_cache(cachepath,buf,i);
					}
					else
					{
						// ---------Cache Miss!------------
						cout << "Cache Miss or Expired field" << endl;
						pivot = strstr(buf+7, "/");

						if(pivot){
							int size = pivot-(buf+7);
							strncpy(ip4, buf+7, size);
							ip4[size] ='\0';
							cout<<"host = "<<ip4<<endl;
						}else{
							strcpy(ip4, buf+7);
							ip4[strlen(ip4)] ='\0';
						}

						hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
						hints.ai_socktype = SOCK_STREAM;

						if ((rv = getaddrinfo(ip4, "http", &hints, &servinfo)) != 0) {
							fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
							exit(1);
						}

						// loop through all the results and connect to the first we can
						for(p = servinfo; p != NULL; p = p->ai_next){
							if ((web_sockfd = socket(p->ai_family, p->ai_socktype,
									p->ai_protocol)) == -1){
								perror("socket");
								continue;
							}
							if (connect(web_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
								close(web_sockfd);
								perror("connect");
								continue;
							}
							break; // if we get here, we must have connected successfully
						}

						freeaddrinfo(servinfo);

						if(expflag == 1)
						{
						// send GET to web
							memset(GETbuf, 0, MAXDATASIZE);
							strcpy(GETbuf, "GET ");
							strcat(GETbuf, buf);
							strcat(GETbuf, " HTTP/1.0\n");
							strcat(GETbuf, "If-Modified-Since: ");
							strcat(GETbuf, cacheexp);
							strcat(GETbuf, "\r\n\r\n");
							cout << GETbuf <<endl;
							cout << cacheexp <<endl;

						}
						else
						{

							memset(GETbuf, 0, MAXDATASIZE);
							strcpy(GETbuf, "GET ");
							strcat(GETbuf, buf);
							strcat(GETbuf, " HTTP/1.0\r\n\r\n");
						}

						recvn = send(web_sockfd, GETbuf, sizeof(GETbuf), 0);
						cout << GETbuf <<endl;
//						cout<<"server request = "<<recvn<<endl;

						// recv Reponse from web until recvn == 0
						memset(RETbuf, 0, MAXRETSIZE);
						recvn = MAXRETSIZE;
						int r_flag = 1;
						int file_flag = 0;
						while(r_flag){
							int sent;
							recvn = recv(web_sockfd, RETbuf, MAXRETSIZE*sizeof(char), 0);
//							cout<<"<<"<<recvn<<"\t";
//							RETbuf[recvn] = '\0';
//							cout<<"<<"<<RETbuf<<"\n";

							if (strstr(RETbuf, "404 Not Found")){
								cout << "404 Not Found\n";
								sent = send(i, "404 Not Found", 13, 0);
								close(web_sockfd);
								r_flag = 0;
								break;
							}

							if(expflag == 1 && file_flag ==0)
							{
								cout<<"checking latest, server says";
								if(strstr(RETbuf, "304 Not Modified"))
								{
									cout <<"= 304 Not Modified"<<endl;

									send_from_cache(cachepath,buf,i);
									break;
								}
								if(strstr(RETbuf, "200 OK"))
								{
									cout <<"= 200 OK"<<endl;
								}
							}
//
							if (recvn == 0){
								cout<<"Finished transfer\n\n"<<endl;
								close(web_sockfd);
								r_flag = 0;
								break;
							}
							if(!file_flag){  // first time
								file_flag = 1;

								struct stat st = {0};

								if (stat("cachefolder", &st) == -1) {
									mkdir("cachefolder", 0700);
								}
								cout<<"fetching data from the server\n";
								strcpy(path, "cachefolder/");
								strcat(path, strrchr(buf, '/')+1); // initialize the path for recv file

								strncpy(exp, strstr(RETbuf, "Expires: ") + 9, 29);
//								cout<<RETbuf<<endl;
								// add this entry in the cache
								//Entry ent(URLstr, path);
								Entry *ent = new Entry ();
								ent->url = URLstr;
								strcpy(ent->path, path);
								strcpy(ent->expire, exp);
								mycache.AddEnt(ent);
								mycache.PrintCache();

								if(strstr(path, ".txt")){
									// begin to write into file
									//cout << "RETbuf: \n" << RETbuf <<endl;
									txtfile = fopen(path, "w");
									if (txtfile == NULL){
										fprintf(stderr, "Can't open output file %s!\n",	path);
										exit(1);
									}
									fputs(strstr(RETbuf, "\r\n\r\n")+4, txtfile);
								}else if(strstr(path, ".jpg")){
									txtfile = fopen(path, "wb");
									if (txtfile == NULL){
										fprintf(stderr, "Can't open output file %s!\n",	path);
										exit(1);
									}
									fwrite(strstr(RETbuf, "\r\n\r\n")+4, 1, recvn - (strstr(RETbuf, "\r\n\r\n") + 4 - RETbuf), txtfile);
								}
							}else{
								if(strstr(path, ".txt")){
									fputs(RETbuf, txtfile);
								}else if(strstr(path, ".jpg")){
									fwrite(RETbuf, 1, recvn, txtfile);
								}
							}
							// send Reponse to client
							sent = send(i, RETbuf, recvn, 0);
//							cout<<">> "<< sent <<" ";
						}
						if(recvn == 0){
							fclose(txtfile);
						}
						memset(RETbuf, 0, MAXRETSIZE);
						memset(buf, 0, MAXDATASIZE);
						memset(GETbuf, 0, MAXDATASIZE);
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
						break;
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END while(1)

	return 0;
}


