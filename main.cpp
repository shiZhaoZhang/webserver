#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <rlog.h>
#include "httpconn.h"
#include "threadPool.h"
#include "connect_pool.h"
#include "map"

#define MAX_EPOLL_EVENTS 20
#define DEBUG_
/*
创建端口，监听连接
    |
创建内核事件表——使用epoll系列函数，使用ET
    |
*/
int main(int argc, char* argv[]){
    if(argc < 3){
        printf("need filename ip-address port\n");
        return 1;
    }
    //初始化log
    LOG_INIT("logData", "log", 10);
    //ip地址
    char *ip = argv[1];
    //端口
    int port = atoi(argv[2]);
    //创建socket,ipv4.tcp
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        LOG_ERROR("Create socket error %d\n", errno);
        LOG_CLOSE();
        return 1;
    }

    //命名socket
    //创建ipv4地址
    struct sockaddr_in m_addr;
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &m_addr.sin_addr);
    m_addr.sin_port = htons(port);
    //绑定
    int ret = bind(listenfd, (struct sockaddr*)&m_addr, sizeof(m_addr));
    if(ret == -1){
        LOG_ERROR("Socket bind error %d\n", ret);
        LOG_CLOSE();
        return 1;
    }
    //监听
    ret = listen(listenfd, 5);
    if(ret == -1){
        LOG_ERROR("Listen error %d\n", ret);
        LOG_CLOSE();
        return 1;
    }

    //创建epoll内核事件表
    int m_epollfd = epoll_create(10);
    //注册sockfd的读事件，也就是监听
    struct epoll_event socketfd_read;
    socketfd_read.events = EPOLLIN;
    socketfd_read.data.fd = listenfd;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, listenfd, &socketfd_read);

    /*连接数据库->创建数据库连接池->创建线程池*/
    //服务器主循环，持续等待epollfd的事件
    struct epoll_event events[MAX_EPOLL_EVENTS];
    //创建数据库池
    connection_pool::GetInstance()->init("localhost", 3306, "zsz", "123456zx","zsz_data", 10);
    //创建线程池
    threadPool<http> m_threadPool(connection_pool::GetInstance(), 4, 20);
    //存储连接过来的sockfd和对应的http
    std::map<int, std::shared_ptr<http>> m_data;
    while(true){
        int nums = epoll_wait(m_epollfd, events, MAX_EPOLL_EVENTS, -1);
        if(nums == -1){
            LOG_ERROR("epoll wait error: %d\n", errno);
            LOG_CLOSE();
            return 1;
        }
        for(int i = 0; i != nums; ++i){
            //新连接
            if(events[i].data.fd == listenfd){
                //接受新连接
                struct sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                int new_con = accept(listenfd, (sockaddr *)&addr, &addrlen);
                if(new_con == -1){
                    LOG_ERROR("accept error, errno = %d",errno);
                    continue;
                }
                auto iter = m_data.find(new_con);
                if(iter == m_data.end()){
                    auto temp_http = std::make_shared<http>(addr);
                    //注册失败,关闭socket
                    if(!temp_http->init(m_epollfd, new_con)){
                        close(new_con);
                        LOG_ERROR("add epoll error,client[%s]\n", inet_ntoa(addr.sin_addr));
                        continue;
                    }
                    m_data[new_con] = temp_http;
                    LOG_INFO("accept new client[%s]", inet_ntoa(addr.sin_addr));
                } else {
                    m_data[new_con]->clearData();
                }
                //设置为非阻塞
                http::setnonblocking(new_con);
                http::epollAdd(m_epollfd, new_con, EPOLLIN); 
                LOG_INFO("Connect socket[%d], clients[%s]", new_con, inet_ntoa(addr.sin_addr));
            } 
            //读事件
            else if(events[i].events & EPOLLIN){
                int m_fd = events[i].data.fd;
                READ_STAT r_stat = m_data[m_fd]->read_once();
                switch (r_stat)
                {
                //读取的数据有误，直接丢弃，重新触发读事件
                case READ_STAT::BAD:
                {
                    m_data[m_fd]->clearData();
                    //重新触发读事件
                    http::modfd(m_epollfd, m_fd, EPOLLIN);
                    continue;
                    break;
                }
                //对方关闭连接
                case READ_STAT::CLOSE:
                {
                    //关闭socket，从epoll内核事件表中移除，从m_data中移除
                    close(m_fd);
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_fd, 0);
                    m_data.erase(m_fd);
                    continue;
                    break;
                }
                case READ_STAT::OK:
                {
                    break;
                }
                default:
                    continue;
                    break;
                }
                //放入请求队列
                if(!m_threadPool.append(m_data[m_fd])){
                    LOG_INFO("Add workList error.");
                    //清除数据，重新注册读事件
                    m_data[m_fd]->clearData();
                    http::modfd(m_epollfd, m_fd, EPOLLIN);
                    continue;
                }
                LOG_INFO("deal with client: %s\n", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
            #ifdef DEBUG_
                printf("deal with client: %s\n", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
            #endif
            }
            //写事件
            else if(events[i].events & EPOLLOUT) {
            #ifdef DEBUG_
                printf("in write\n");
            #endif
                int m_fd = events[i].data.fd;
                //write_once == false代表要关闭socket
                if(!m_data[m_fd]->write_once()){
                    LOG_INFO("Close socket, client[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
                    close(m_fd);
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_fd, 0);
                    m_data.erase(m_fd);
                }
            }
        }
    }
}