#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
//创建IPv4
class createSockfd{
    struct sockaddr_in address;
public:
    int sockfd;    
    
    struct sockaddr_in client;
    socklen_t client_addrlength;
    int connfd;
    std::vector<int> connfd_v;   //accept接受到的socketfd
public:
    createSockfd(const char *ip, int port){
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(sockfd >= 0);
        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        assert(inet_aton(ip, &address.sin_addr) == 1);

        client_addrlength = sizeof(client);
    }
    ~createSockfd(){
        close(sockfd);
        close(connfd);
        /*for(std::vector<int>::iterator i = connfd_v.begin(); i != connfd_v.end(); ++i){
            close(*i);
        }*/
        
    }
    int bindSockfd();
    int connectSockfd();
    int listenfd(int backlog = 5);
    int acceptfd();
};

//命名socket
int createSockfd::bindSockfd(){
    return bind(sockfd, (struct sockaddr*)&address, sizeof(address));

}

//连接socket
int createSockfd::connectSockfd(){
    return connect(sockfd, (struct sockaddr*)&address, sizeof(address));
}

//监听socket
int createSockfd::listenfd(int backlog){
    return listen(sockfd, backlog);
}

//接受连接
int createSockfd::acceptfd(){
    connfd = accept(sockfd, (struct sockaddr*)&client, &client_addrlength);
    connfd_v.push_back(connfd);
    return connfd;
}

//把文件描述符设置为非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将文件描述符fd上的可读事件EPOLLIN注册到内核事件表中，参数enable_et表示是否开启ET模式
void addfdET(int epollfd, int fd, bool enable_et){
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    if(enable_et){
        event.events |= EPOLLET;
    }
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    if(ret == -1){
        printf("epoll ctl error: errno:%d\n", errno);
        exit(0);
    }
    setnonblocking(fd);
}
//将文件描述符fd上的可读事件EPOLLIN注册到内核事件表中，参数enable_shot表示是否开启EPOLLONESHOT模式
void addfdONESHOT(int epollfd, int fd, bool enable_shot){
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;
    if(enable_shot){
        event.events |= EPOLLONESHOT;
    }
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    if(ret == -1){
        printf("epoll ctl error: errno:%d\n", errno);
        exit(0);
    }
    setnonblocking(fd);
}
//重新注册EPOLLONESHOT
void reset_oneshot(int epollfd, int fd){
    epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}