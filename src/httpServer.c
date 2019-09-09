#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define PORT 8080
#define MAX_FD 1000
#define MAX_LINE 102400
#define EPOLLEVENTS 1000

//PHP路径
#define PHP "/usr/local/php/bin/php /data/c/bin/index.php"

void myerror(char msg[]);
void httpdata(char* buf, char* cgi);

int main(int argc, char* argv){
    int sockfd, connfd, epollfd, nfds;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t clientlen;
    char buf[MAX_LINE];
    char msg[MAX_LINE];
    struct epoll_event ev, events[EPOLLEVENTS] ;
    ssize_t nread;

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        myerror("create socket myerror");
    }
    if((bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))) == -1){
        myerror("bind sock myerror");
    }
    if(listen(sockfd, MAX_FD)){
        myerror("listen sock myerror");
    }
    
    epollfd = epoll_create(MAX_FD);
	if(epollfd == -1){
        myerror("epoll_create myerror");
    }
    
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sockfd;

    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1){
        myerror("epoll_ctl myerror");
    }
   
    while(1){
        nfds = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
                
        for(int i = 0; i < nfds; i++){
            connfd = events[i].data.fd;
            if(connfd <= 0){
                continue;
            }
            if(events[i].events & (EPOLLERR | EPOLLHUP)){ 
                continue;
            }else if(connfd == sockfd){//有新的连接
                connfd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientlen);
                ev.data.fd = connfd;
                ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
                
                printf("accept a new client: %s\n", inet_ntoa(clientaddr.sin_addr));
            
            }else if(events[i].events & EPOLLIN){//有读数据
                fcntl(connfd, F_SETFL, O_NONBLOCK);
                bzero(buf, sizeof(buf));
                while(1){
                    nread = read(connfd, buf, MAX_LINE);
                    if(nread < 0){
                        if(errno == EINTR){
                            nread = 0;
                        }else{
                            break;
                        }
                    }else if(nread == 0){
                        close(connfd);
                        break;
                    }
                }
                printf("<<<<<< : %s\n", buf);
                ev.data.fd = connfd;
                ev.events = EPOLLOUT;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);

            }else if(events[i].events & EPOLLOUT){//有写数据
                bzero(buf, sizeof(buf));
                
                char cgi[102400];
                
                FILE * fp;
                fp = popen(PHP, "r");
                fread(cgi, 1, sizeof(cgi), fp);
                
                httpdata(buf, cgi);
                
                printf(">>>>>>: %d \n", strlen(buf));
                printf("%s \n", buf);
                write(connfd, buf, strlen(buf));
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
            }
        }
    }
  
    return 0;
}

void myerror(char msg[]){
    printf("%s\n", msg);
    exit(-1);
}

void httpdata(char* buf, char* cgi){
    strcat(buf, "HTTP/1.1 200 OK\r\n");
    strcat(buf, "Server: myHttpServer 1.0\r\n");
    strcat(buf, "Connection: close\r\n");
    strcat(buf, "Content-Type: text/html\r\n");
    char header[1024];
    sprintf(header, "Content-Length: %d\r\n\r\n", strlen(cgi)); 
    strcat(buf, header);
    strcat(buf, cgi);
}


