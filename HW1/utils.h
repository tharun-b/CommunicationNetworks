//
//  utils.h
//  mainly deals with 2D pointers operations
//
//  Created by Guoyu Fu on 9/18/14.
//
//

#ifndef _utils_h
#define _utils_h

#include <iostream>
#include <string>
#include <ctime>

using namespace std;

//********************** 1 SBCP CONSTANTS****************************

// 1.1 Message Type
#define JOIN 2
#define FWD 3
#define SEND 4
#define NAK 5
#define OFFLINE 6
#define ACK 7
#define ONLINE 8
#define IDLE 9

#define DEBUG_FLAG 0

#define MAX(a,b) ((a>b)?a:b)
#define MIN(a,b) ((a<b)?a:b)
// 1.2 Attribute Type
#define REASON 1
#define USERNAME 2
#define CLIENTCOUNT 3
#define MESSAGE 4

// 1.3 Others
#define VRSN 3 // protocol version is 3
#define MAXIDLEINTV 10 // maximal idle interval
#define MAXCLNT 10 // maximal numbers of clients
#define MSG_ILLEGAL -1 // to represent a return value if the msg is illegal
#define MAXCHATSIZE 512 // max number of bytes we can SEND at once

//********************** 2 SBCP STRUCTS******************************

struct msg{
    unsigned int vrsn: 9;
    unsigned int type: 7;
    unsigned short length;
    
    unsigned short atype;
    unsigned short alength;
    
    union{
        char username[ ];
        char message[ ];
        char reason[ ];
        short clnt_count; // client count
    };
};

struct msg_FWD{
    unsigned int vrsn: 9;
    unsigned int type: 7;
    unsigned short length;
    
    unsigned short atype1;
    unsigned short alength1;
    char username[16];
    
    unsigned short atype2;
    unsigned short alength2;
    char message[512];
    
};

//********************** 3 FUNCTIONS ******************************

int convert_data(uint32_t *buf, uint32_t *host_aligned_data ,int size)
{
    {
        uint32_t i;
        uint32_t *ptr;
        uint32_t *ptr2;
        ptr = (uint32_t *)buf;
        ptr2 = host_aligned_data;
        
        for (i= 0; i < (uint)size; i+= 4)
        {
            *ptr2 = ntohl(*ptr);
#if DEBUG_FLAG
            printf("recieved %x\t",*ptr);
            printf("converted %x\n",*ptr2);
#endif
            ptr++;
            ptr2++;
        }
    }
    return 0;
}

int clnt_msg(int msgtype, char *text, uint32_t *buffer_aligned)
{
    
    // check if the msg type is illegal
    if (msgtype != JOIN && msgtype != SEND && msgtype != IDLE ) {
        cout << "CLNT_MSG: Message Type is Illegal." << endl;
        return MSG_ILLEGAL;
    }
    
    // initiate the structs
    msg *mymsg;
    char buffer[600];
    int text_size = strlen(text);
    
    
    uint32_t size = (8 + text_size*sizeof(char));
    size = ((size + 3)>>2)<<2;
#if DEBUG_FLAG
    printf("text_size = %d\d"
    		"total_size = %d",text_size,size);
#endif
    mymsg = (msg*)buffer;

    //mymsg = (msg*)malloc(sizeof(msg) + strlen(text)*sizeof(char));
    mymsg->vrsn = VRSN;
    mymsg->type = msgtype;
    
    // assign attribute to different type of msg
    switch (msgtype) {
        case JOIN:
            mymsg->atype = USERNAME;
            
            // check if the text being sent is too long
            if (text_size > 16){
                cout << "CLNT_MSG: JOIN username is too long (> 16 bytes)." << endl;
                return MSG_ILLEGAL;
            }
            
            strcpy(mymsg->username, text);
            mymsg->alength = text_size + 4;
            mymsg->length = ((mymsg->alength + 7)>>2)<<2;
            break;
        case SEND:
            mymsg->atype = MESSAGE;
            
            // check if the text being sent is too long
            if (strlen(text) > 512){
                cout << "CLNT_MSG: SEND text is too long (> 512 bytes)." << endl;
                return MSG_ILLEGAL;
            }
            
            memcpy(mymsg->message, text, text_size);
            mymsg->alength = text_size + 4;
            mymsg->length = ((mymsg->alength + 7)>>2)<<2;
            
            break;
        case IDLE:
            mymsg->atype = USERNAME;
            
            // check if the text being sent is too long
            if (strlen(text) > 16){
                cout << "CLNT_MSG: IDLE username is too long (> 16 bytes)." << endl;
                return MSG_ILLEGAL;
            }
            
            strcpy(mymsg->username, text);
            mymsg->alength = strlen(mymsg->username) + 4;
            mymsg->length = ((mymsg->alength + 7)>>2)<<2;
            break;
        default:;
    }
    
    //cout << "CLNT_MSG: " << mymsg->vrsn << " " <<mymsg->type<< " "<<mymsg->length<< " "<<mymsg->message<< " "<<mymsg->alength<< endl;
    /*memcpy(text, mymsg, mymsg->length);
    int len;
    len = mymsg->length;
    memset(mymsg, 0, len);
    return len;
    */
    
    //int ret_val = mymsg->length;
    uint32_t i;
    uint32_t *ptr , *ptr2;
    ptr = (uint32_t *)mymsg;
    ptr2 = buffer_aligned;
    
//    printf("size = %d\n",size);
    for (i= 0; i < size; i+= 4)
    {
        *ptr2 = htonl(*ptr);
#if DEBUG_FLAG
        printf("source %x\t",*ptr);
        printf("converted %x\n",*ptr2);
#endif
        ptr++;
        ptr2++;
    }
    
    
    return size;
}

int svr_msg(int msgtype, char *text,uint32_t text_size, char *text2,uint32_t text2_size, uint32_t *buffer_aligned)
{
    
    // check if the msg type is illegal
    if (msgtype != ACK && msgtype != NAK && msgtype != ONLINE && msgtype != FWD && msgtype != IDLE && msgtype != OFFLINE ) {
        cout << "SVR_MSG: Message Type is Illegal." << endl;
        return MSG_ILLEGAL;
    }
    
    
    uint32_t i;
    uint32_t *ptr , *ptr2;
    uint32_t size=0;
    
    switch (msgtype) {
            /*
        case IDLE:
            msg mymsg;
            
            mymsg.vrsn = VRSN;
            mymsg.type = msgtype;
            mymsg.atype = USERNAME;
            
            // check if the text being sent is too long
            if (strlen(text) > 16){
                cout << "SVR_MSG: IDLE text is too long (> 16 bytes)." << endl;
                return MSG_ILLEGAL;
            }
            
            memcpy(mymsg.username, text, strlen(text));
            mymsg.alength = strlen(mymsg.username) + 4;
            mymsg.length = mymsg.alength + 4;
            
            memcpy(text, &mymsg, mymsg.length);
            
            ptr = (uint32_t *)mymsg;
            ptr2 = buffer_aligned;
            
            printf("size = %d\n",size);
            for (i= 0; i < size; i+= 4)
            {
                *ptr2 = htonl(*ptr);
                printf("source %x\t",*ptr);
                printf("converted %x\n",*ptr2);
                ptr++;
                ptr2++;
            }
            printf("buf: %x\n",buffer_aligned);

            
            len = mymsg.length;
            break;
             */
        case FWD:// text for username, text2 for msg
            msg_FWD mymsg_FWD;
            
            memset(&mymsg_FWD,0,sizeof(mymsg_FWD));
            mymsg_FWD.vrsn = VRSN;
            mymsg_FWD.type = msgtype;
            
            if (text2 != NULL && text != NULL) {
                mymsg_FWD.atype1 = USERNAME;
                
                // check if the text being sent is too long
                if (text_size > 16){
                    cout << "SVR_MSG: FWD text is too long (> 16 bytes)." << endl;
                    return MSG_ILLEGAL;
                }

//                printf("text = %d,\ntext_2 = %d,\n",text_size,text2_size);
                memcpy(mymsg_FWD.username, text, text_size);
//                mymsg_FWD.username[text_size] ='\0';
                mymsg_FWD.alength1 = text_size + 4;
                mymsg_FWD.atype2 = MESSAGE;
                
                // check if the text being sent is too long
                if (text2_size> 512){
                    cout << "SVR_MSG: FWD text is too long (> 512 bytes)." << endl;
                    return MSG_ILLEGAL;
                }
//                printf("%s\n",text);
//                printf("%s\n",text2);
                
                memset(mymsg_FWD.message,0,text2_size);
                memcpy(mymsg_FWD.message,text2, text2_size);
                //mymsg_FWD.message[strlen(text2)] = '\0';
                //mymsg_FWD.message[strlen(text2)] = '\0';
                mymsg_FWD.alength2 = text2_size + 4;
                size = mymsg_FWD.length = ((28 + mymsg_FWD.alength2)>>2)<<2;
                
                //memset(text, 0, strlen(text));
                //memset(text2, 0, strlen(text2));
                
               // memcpy(text, &mymsg_FWD, sizeof(msg_FWD));
                ptr = (uint32_t *)&mymsg_FWD;
                ptr2 = buffer_aligned;
                
//                msg_FWD *svrmsg;
//                svrmsg = (msg_FWD*)ptr;
                //cout << endl <<"SERVER: " << svrmsg->vrsn << " " <<svrmsg->type<< " "<<svrmsg->length<< " "<<svrmsg->atype<< " "<<svrmsg->alength  << endl;
                
//                cout << endl << svrmsg->username << ": "<<svrmsg->vrsn<< " "<<svrmsg->type<< " "<<svrmsg->length<< " "<<svrmsg->atype1<< " "<<svrmsg->alength1<< " "<<svrmsg->username<< " "<<svrmsg->atype2<< " "<<svrmsg->alength2<< " "<<svrmsg->message<<endl;
//                printf("size = %d\n",size);
                for (i= 0; i < size; i+= 4)
                {
                    *ptr2 = htonl(*ptr);
#if DEBUG_FLAG

                    printf("source %x\t",*ptr);
                    printf("converted %x\n",*ptr2);
#endif
                    ptr++;
                    ptr2++;
                }
                
                
            }
            //len = mymsg_FWD.length;
            break;
        default:
            break;
            return -1;
    }
    
    
    //cout << "SVR_MSG: " << mymsg->vrsn << " " <<mymsg->type<< " "<<mymsg->length<< " "<<mymsg->message<< " "<<mymsg->alength<< endl;
    
    //memset(mymsg, 0, len);
    
    //int ret_val = mymsg->length;
    
    
    
    
    
    //free(mymsg);
    
    return size;
    
    //return len;
    
}

//reference: http://www.hankcs.com/program/cpp/c__c___input_any_indefinite_length_of_string.html
#define STEP 1


// TODO:  free str
char * getstr()    //return the address of strings from commandline with any length
{
    std::clock_t startTime, endTime, clockTicksTaken; // time marks for IDLE check, reference: http://stackoverflow.com/questions/3220477/how-to-use-clock-in-c
    double timeInSeconds;
    startTime = clock();
    
    char *temp, *str=(char *)malloc(1);
    memset(str,0,1);
    int c=0, len=0, times=1, number=0;
    if(!str)
    {
        printf("not enough memory");
        return (char *)NULL;
    }
    number += times*STEP;
    while((c=getchar())!=10)   // runs until "enter" key is pressed
    {
        // check IDLE
        endTime = clock();
        clockTicksTaken = endTime - startTime;
        timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
        if (timeInSeconds > MAXIDLEINTV){
            strcpy(str, "idle");
            return str;
        }
        
        if(len==number)
        {
            times++;
            number=times*STEP;
            temp=str;
            str=(char *)realloc(str,number);
            if(str==NULL)
            {
                printf("memory is not enough");
                str=temp;
                break;
            }
        }
        *(str+len)=c;
        len++;
    }
    str=(char *)realloc(str,len+1);
    *(str+len)='\0';
    
    return str;
}


#endif
