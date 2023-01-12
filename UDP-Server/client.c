#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define IP "127.0.0.1"
#define SPORT 8266
int socketFD = -1;

unsigned short port;

enum flag{
    LOGIN,QUIT,MESSAGE
};

typedef struct client_message{
    enum flag FLAG;
    char client_name[16];
    char client_msg[1024];
}cmessage;

void *receive(void *arg){
    int ret=-1;
    struct sockaddr_in recv_addr;
    socklen_t recvlen = sizeof(recv_addr);
    // struct timeval tv_out;
    // tv_out.tv_sec=1;//等待1秒
    // tv_out.tv_usec=0;
    // if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv_out, sizeof(tv_out)) < 0)
    //     perror("setsockopt failed:");
    while(1){
        char ip[32];
        cmessage cmsg;
        int size = sizeof(cmsg);
        memset(ip,'\0',sizeof(ip));
        port = 0;
        ret = recvfrom(socketFD,&cmsg,size,0,(struct sockaddr*)&recv_addr,&recvlen);
        strcpy(ip,inet_ntoa(recv_addr.sin_addr));
        port = ntohs(recv_addr.sin_port);
        if(ret>=0){
            printf("\n%s: %s\n",cmsg.client_name,cmsg.client_msg);

        }
            // printf("[receive from %s:%hu]%s \n",ip,port,msg);
        else
            perror("ERROR");
    }
}
void main(){
    int bind_ret;
    unsigned short client_port;
    char name[16];
    cmessage client_data;
    socketFD = socket(AF_INET,SOCK_DGRAM,0);
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SPORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);
    
    memset(name,'\0',sizeof(name));
    printf("Please enter your name\n> ");
    scanf("%s",name);
    printf("Hello %s!\nPlease enter your port\n> ",name);
    scanf("%hu",&client_port);
    printf("Your port is:%hu!\n",client_port);

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(client_port);
    client_addr.sin_addr.s_addr = inet_addr(IP);
    
    socklen_t caddr_len = sizeof(client_addr);
    bind_ret = bind(socketFD,(struct sockaddr*)&client_addr,caddr_len);
    if(bind_ret < 0){
        perror("Bind failed");
        close(socketFD);
        return;
    }

    strcpy(client_data.client_name,name);
    strcpy(client_data.client_msg,"TEST");
    client_data.FLAG=LOGIN;
    int login_ret;
    login_ret = sendto(socketFD,&client_data,sizeof(client_data),0,(struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("Waiting for login into server...\n");
    sleep(1);
    if(login_ret < 0){
        perror("Login failed");
        close(socketFD);
        return;
    }
    
    pthread_t t;
    
    int create_ret = pthread_create(&t,NULL,receive,NULL);
    if(create_ret != 0)
        perror("create failed");

    int det_ret = pthread_detach(t);
    if(det_ret != 0)
        perror("detach failed");
    // pthread_join(t,NULL);

    // system("clear");
    printf("Login successfully!\n");
    char sendmsg[1024];
    while(1){
        usleep(100000);
        memset(sendmsg,'\0',sizeof(sendmsg));
        printf("Please enter your message\n> ");
        scanf("%s",sendmsg);
        strcpy(client_data.client_msg,sendmsg);
        client_data.FLAG=MESSAGE;
        if(strcmp(sendmsg,"quit")==0){
            client_data.FLAG=QUIT;
            sendto(socketFD,&client_data,sizeof(client_data),0,(struct sockaddr*)&server_addr, sizeof(server_addr));
            break;
        }
        sendto(socketFD,&client_data,sizeof(client_data),0,(struct sockaddr*)&server_addr, sizeof(server_addr));
    }  
    close(socketFD);

}