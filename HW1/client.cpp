/*
 ** client.c
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
#include <iostream>
#include <arpa/inet.h>
#include <ctime>
#include "utils.h"

#define STDIN 0 // for standard input

using namespace std;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main (int argc, char *argv[]){
    
    if (argc != 4) {
        cout << "Input parameters are not equal to 4." <<endl;
        exit(1);
    }
    
    char * myname = NULL;
    myname = (char *)malloc(sizeof(char)*(strlen(argv[1])+1));
    strcpy(myname, argv[1]);
    myname[strlen(argv[1])] = '\0';
    
    uint32_t *aligned_data = (uint32_t *)malloc(600);
    //TODO:free this
    
    // check if input parameters are enough
    if (argc != 4) {
        fprintf(stderr,"usage:user_name server_IP server_port\n");
        exit(1);
    }
    
    // specify the settings of TCP socket
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    // specify the IP address
    struct addrinfo *servinfo;
    int rv;
#if DEBUG_FLAG
    printf("the port number from command line is %s\n",argv[2]);
#endif
    if ((rv = getaddrinfo(argv[2],argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results (TODO: what results?) and connect to the first we can
    struct addrinfo *p;
    int sockfd;
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break; // TODO: What is this break for?
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    // translate the network address given by 'get_in_addr' to printable ip4 in 's'
    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
//    printf("client: Sending Request to join to %s\n", s);
    
    freeaddrinfo(servinfo); // all done with this structure
    /*
     if (send(sockfd, argv[1], strlen(argv[1]), 0) == -1)
     perror("send");
     */
    //fflush (stdout);
    
    // JOIN the server
    /*
    char * tmpname = NULL;
    tmpname = (char *)malloc(sizeof(char)*strlen(argv[1]));
    strcpy(tmpname, argv[1]);
    */
    uint32_t * tmpdata = NULL;
    
    
    int sendr,tmp;
    tmpdata = (uint32_t*)malloc(MAXCHATSIZE + 8);
    memset(tmpdata,0,MAXCHATSIZE + 8);
    
    
    tmp = clnt_msg (JOIN, argv[1], tmpdata); // return the msg length in byte
#if DEBUG_FLAG
    printf("after:%x\n",tmpdata);
    printf("size returned:%x\n",tmp);
#endif
    sendr = send(sockfd, tmpdata, tmp, 0);
    free(tmpdata);
    if (sendr == -1){
        perror("JOIN_send");
        close(sockfd);
    }
    
    // dealing with select()
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;
    FD_ZERO(&master); // clear set
    FD_ZERO(&read_fds); // clear set
    FD_SET(STDIN, &master); // add standard input to list
    FD_SET(sockfd, &master); // add TCP connection to list
    fdmax = sockfd;
    
    int nbytes=0;
    char  buf[600];
    char rbuf[600];
    
    cout<<"Hi! "<<myname<<" You can type now." <<endl;
    
    while (1) {
        
        read_fds = master; // copy list of sets to tmplist
        
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        
        if (FD_ISSET(STDIN, &read_fds)) {
            
        	memset(buf,0,600);
        	char *data = getstr();

            strncpy(buf, data, strlen(data)+1);
            free(data);
//        	cin >> buf;
//        	buf[strlen(buf)] = '\0';
            nbytes = strlen(buf);

            if (nbytes != 0){
                if (nbytes > MAXCHATSIZE){
                    std::cout << "The words you typed are too long. Please retype."<< endl;
                    memset(buf, 0, strlen(buf));
                    std::cout << argv[1] << ": ";
                }
                else { // here is a legal input
                    // wrap this input up
                    //tmp = clnt_msg (SEND, buf);
                    //sendr = send(sockfd, buf, tmp, 0);
                    
                    memset(tmpdata,0,MAXCHATSIZE + 8);
                    tmp = clnt_msg (SEND, buf,tmpdata);
                    //                    tmp = clnt_msg (SEND, buf,tmpdata);
                    sendr = send(sockfd, tmpdata, tmp, 0);
                    
                    if (sendr == -1){
                        perror("send");
                        close(sockfd);
                    }
//                    cout << argv[1] << ": ";
                    memset(buf, 0, tmp);
                }
            }
        }
        
        if (FD_ISSET(sockfd, &read_fds)) {
            nbytes = recv(sockfd, rbuf, 600, 0);
            if (nbytes == -1) {
                perror("recv");
                exit(1);
            }// TODO: nbytes == 0, means server hung up
            
            if (nbytes == 0) {
                cout<<"Chat room is closed\n"<<endl;
                exit(1);
            }
            //rbuf[nbytes] = '\0';
//            cout<< "nbytes: "<<nbytes<<endl;
            
            //cout << endl << "nbytes: "<<nbytes;
            memset(aligned_data, 0, nbytes);
            convert_data((uint32_t *)rbuf,aligned_data, nbytes);
            
            msg_FWD *svrmsg;
            svrmsg = (msg_FWD*)(aligned_data);
            
//            cout << endl << svrmsg->username << ": "<<svrmsg->vrsn<< " "<<svrmsg->type<< " "<<svrmsg->length<< " "<<svrmsg->atype1<< " "<<svrmsg->alength1<< " "<<svrmsg->username<< " "<<svrmsg->atype2<< " "<<svrmsg->alength2<< " "<<svrmsg->message<<endl;
            
            switch (svrmsg->type)
            {
            case FWD:
            {
            uint chat_length;
            /* using this buf temporarily */
            chat_length = MAX((svrmsg->alength2-4),0);
//            printf("chat_length  %d",chat_lengtsh);
            strncpy(rbuf,svrmsg->message,chat_length);
            rbuf[chat_length] ='\0';
#if DEBUG_FLAG
            cout << endl << svrmsg->username << ": "<<svrmsg->vrsn<< " "<<svrmsg->type<< " "<<svrmsg->length<< " "<<svrmsg->atype1<< " "<<svrmsg->alength1<< " "<<svrmsg->username<< " "<<svrmsg->atype2<< " "<<svrmsg->alength2<< " "<<svrmsg->message<<endl;
            //cout << endl<< svrmsg->username<<": " << svrmsg->message<<endl;
#endif
            printf("%s:%s\n",svrmsg->username,rbuf);

            break;
            }
            default:
            	break;
            }
            
            memset(svrmsg, 0, svrmsg->length);
            memset(rbuf, 0, 600);
            
        }
    }
    
    free (aligned_data);
    close(sockfd);
    return 0;
}

/*
 std::clock_t startTime = clock();
 // timer function referenced from:
 //http://stackoverflow.com/questions/3220477/how-to-use-clock-in-c
 std::clock_t endTime = std::clock();
 std::clock_t clockTicksTaken = endTime - startTime;
 double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
 
 // TODO: if timeInSeconds > MAXIDLEINTV, send IDLE
 
 */





/*
 if (strcmp(buf, "idle") == 1) {
 tmpname = (char *)malloc(sizeof(char)*strlen(argv[1]));
 strcpy(tmpname, argv[1]);
 tmp = clnt_msg (IDLE, tmpname);
 sendr = send(sockfd, tmpname, tmp, 0);
 free(tmpname);
 if (sendr == -1){
 perror("IDLE_send");
 close(sockfd);
 }
 }
 */



/* For debugging
 // here are the msg being sent
 msg *urmsg;
 urmsg = reinterpret_cast<msg*>(buf);
 cout << "client: " << urmsg->vrsn << " " <<urmsg->type<< " "<<urmsg->length<< " "<<urmsg->message<< " "<<urmsg->alength  << endl;
 */















