/*
 ** server.c -- a stream socket server demo
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
#include "utils.h"
#include <unistd.h>
using namespace std;

#define MAXDATASIZE 520

//#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
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
    if (argc != 4) {
        cout << "Input parameters are not equal to 4." <<endl;
        exit(1);
    }
    
    struct addrinfo hints, *servinfo, *p;
    //    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    int rv;
    
    //char buf[MAXDATASIZE];
    int listener_sockfd;// listening socket descriptor
    int client_sockfd;   // New client sockfd
    struct sockaddr_storage their_addr; // client address
//    char remoteIP[INET6_ADDRSTRLEN];
    
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
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((listener_sockfd = socket(p->ai_family, p->ai_socktype,
                                      p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }
        
        if (setsockopt(listener_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
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
    
    if (listen(listener_sockfd, BACKLOG) == -1)
    {
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
    
    printf("Chat room created\n");
    
    // add the listener to the master set
    FD_SET(listener_sockfd, &master);
    
    // keep track of the biggest file descriptor
    fdmax = listener_sockfd; // so far, it's this one
    
    char buf[MAXDATASIZE], buf2[MAXDATASIZE], buf3[MAXDATASIZE];
    char *namelist[atoi(argv[3])+5]; // array to store client names (first 5 are primitive)
    for (int tmp=0; tmp < atoi(argv[3])+5 ; tmp++)
        namelist[tmp] = (char*)malloc(16);
   cout<<"Chat room created for "<<atoi(argv[3])<<" clients."<<endl;
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
/*
                        printf("New connection from %s on "
                               "socket %d\n",
                               inet_ntop(their_addr.ss_family,
                                         get_in_addr((struct sockaddr*)&their_addr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               client_sockfd);
                        */

                    }
                }
                else
                {
                    //cout<<"i:" << i << ". fdmax: " << fdmax <<endl;
                	int chat_size;

                    int recvn =0 ;
                    memset(buf,0,MAXCHATSIZE);
                    recvn = recv(i, buf, MAXDATASIZE*sizeof(char), 0);
                    
                    // handle data from a client
                    if (recvn <= 0) {
                        // got error or connection closed by client
                        if (recvn == 0) {
                            // connection closed
                            cout << "Client-" << namelist[i] << "left the chat room." << endl;
                        } else {
                            perror("recv");
                        }
                        
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    
//                    printf("size_r = %d\n",recvn);
                    uint32_t *host_aligned_data = (uint32_t *)calloc(1024,sizeof(char));

                   convert_data((uint32_t *)buf,host_aligned_data, recvn);

//                                cout << endl << svrmsg->username << ": "<<svrmsg->vrsn<< " "<<svrmsg->type<< " "<<svrmsg->length<< " "<<svrmsg->atype1<< " "<<svrmsg->alength1<< " "<<svrmsg->username<< " "<<svrmsg->atype2<< " "<<svrmsg->alength2<< " "<<svrmsg->message<<endl;

                    
                    //cout << "recvn: " <<recvn << endl;
                    msg *urmsg;
                    urmsg = (msg *)(host_aligned_data);
#if DEBUG_FLAG
                    cout << endl << urmsg->username << ": "<<urmsg->vrsn<< " "<<urmsg->type<< " "
                    		<<urmsg->length<< " "
                    		<<urmsg->username<<" "
                    		<<urmsg->alength<< " "
                    		<<urmsg->message<<endl;
#endif

                    memset(buf2, 0, MAXDATASIZE);
                    memset(buf3, 0, MAXDATASIZE);
                    strncpy(buf2, namelist[i], strlen(namelist[i]));
                    chat_size = MAX((urmsg->alength-4),0);
#if DEBUG_FLAG
                    printf("data = %s,\nchat_size = %d\n",urmsg->message,chat_size);
#endif
                    buf3[chat_size] = '\0';

                    memcpy(buf3, urmsg->message, (chat_size+1));
#if DEBUG_FLAG
                    printf("buf3 = %s,\nnew chat size = %d\n",buf3,(chat_size+1));
#endif
//                    urmsg->message[urmsg->alength-3] = '\0';

                    //cout << "client says: " << urmsg << " "<< endl;
#if DEBUG_FLAG
                    cout << endl << namelist[i] << ": "<<urmsg->vrsn<< " "<<urmsg->type<< " "<<urmsg->length<<endl<<" "<<urmsg->atype<< " "<<urmsg->alength<<endl<< " "<<urmsg->message<<endl;
#endif
                    switch (urmsg->type)
                    {
                        case SEND:
                        {

/*                        	cout << namelist[i] << ": " << urmsg->message << endl;*/

                        	/*   int  tmpsize;
                            tmpsize = (((14+(urmsg->alength-4))+3)>>2)<<2;
                            //tmpdata = (uint32_t*)malloc(MAXCHATSIZE + 8);
                            //tmpdata[]='\0';

                            cout << "tmpsize: " <<tmpsize<<endl;
                        	 */

                        	int l_fwd;
//                        	cout << namelist[i] << "buf3: " << buf3 << endl;

                        	// FWD
                        	uint32_t data_n[256];
                        	uint32_t *tmpdata = data_n;

                        	memset(tmpdata,0,256*sizeof(int));
                        	l_fwd = svr_msg(FWD, buf2,strlen(namelist[i]),buf3,chat_size,tmpdata);
//                        	cout << "l_fwd: " <<l_fwd<<endl;



#if DEBUG_FLAG
                        	 msg_FWD *svrmsg;
                        	 svrmsg = reinterpret_cast<msg_FWD*>(tmpdata);
                        	 //                            cout << endl <<"SERVER: " << svrmsg->vrsn << " " <<svrmsg->type<< " "<<svrmsg->length<< " "<<svrmsg->atype<< " "<<svrmsg->alength  << endl;

                            cout << endl << svrmsg->username << ": "<<svrmsg->vrsn<< " "<<svrmsg->type<<
                            		" "<<svrmsg->length<< " "<<svrmsg->atype1<<
                            		" "<<svrmsg->alength1<< " "<<svrmsg->username<<
                            		" "<<svrmsg->atype2<< " "<<svrmsg->alength2<<
                            		" "<<svrmsg->message<<endl;
#endif
                        	int r, tmp_i;
                        	for(tmp_i = 1; tmp_i <= fdmax; tmp_i++)
                        	{
                        		if (FD_ISSET(tmp_i, &master))
                        			if (tmp_i != listener_sockfd && tmp_i != i)
                        			{
                        				r = send(tmp_i, tmpdata, l_fwd, 0);
                        				if (r == -1)
                        					perror("send");
                        			}
                        	}
                        	break;
                        }
                        case JOIN:
                        {
                            cout << "User-"<< urmsg->username<< "entered the chat room!!!"<< endl;
                            strcpy(namelist[i], urmsg->username);
                            break;
                        }
                        case IDLE:
                        {
                            cout << urmsg->username << "(IDLE): " << endl;
                            break;
                        }
                        default:
                            break;
                    }
                    
                    free(host_aligned_data);
                    memset(buf, 0, MAXDATASIZE);
                    
                    
                    
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END while(1)
    
    
    return 0;
}

/*
 case SEND:
 cout << namelist[i] << ": " << urmsg->message << endl;
 
 // FWD
 int t1, t2;
 memcpy(buf2, namelist[i], strlen(namelist[i]));
 cout<<"len: "<<strlen(namelist[i])<<endl;
 cout<<"buf2_1: "<<buf2<<endl;
 t1 = svr_msg(FWD, buf2, NULL); // buf2 for FWD msg with USERNAME
 cout<<"buf2_2: "<<buf2<<endl;
 
 msg *urbuf2;
 urbuf2 = reinterpret_cast<msg*>(buf2);
 cout<<"urbuf2: "<< urbuf2->vrsn<<" "<<urbuf2->type<<" " << urbuf2->length<<" "<< urbuf2->username << endl;
 memset(urbuf2, 0, urbuf2->length);
 
 
 //cout<<"buf2 len: "<< strlen(buf2)<<endl;
 
 int r1, r2, tmp_i;
 for(tmp_i = 5; fdmax > 5 && tmp_i <= fdmax; tmp_i++)
 if (FD_ISSET(tmp_i, &master))
 if (tmp_i != listener_sockfd && tmp_i != i){
 r1 = send(tmp_i, buf2, t1, 0);
 cout << "sockfd: " << tmp_i << ". r1: " << r1 <<endl;
 if (r1 == -1)
 perror("send");
 }
 memset(buf2, 0, MAXDATASIZE);
 
 
 sleep(5);
 
 memcpy(buf3, urmsg->message, strlen(urmsg->message));
 cout<<"len: "<<strlen(urmsg->message)<<endl;
 cout<<"buf3_1: "<<buf3<<endl;
 
 t2 = svr_msg(FWD, NULL, buf3); // buf3 for FWD msg with message
 cout<<"buf3_2: "<<buf3<<endl;
 
 msg *urbuf3;
 urbuf3 = reinterpret_cast<msg*>(buf3);
 cout<<"urbuf3: "<< urbuf3->vrsn<<" "<<urbuf3->type<<" " << urbuf3->length<<" "<< urbuf3->username << endl;
 memset(urbuf3, 0, urbuf3->length);
 
 for(tmp_i = 5; fdmax > 5 && tmp_i <= fdmax; tmp_i++)
 if (FD_ISSET(tmp_i, &master))
 if (tmp_i != listener_sockfd && tmp_i != i){
 r2 = send(tmp_i, buf3, t2, 0);
 cout << "sockfd: " << tmp_i << ". r2: " << r2 <<endl;
 if (r2 == -1)
 perror("send");
 }
 
 memset(urmsg, 0, urmsg->length);
 
 memset(buf3, 0, MAXDATASIZE);
 
 break;
 */
