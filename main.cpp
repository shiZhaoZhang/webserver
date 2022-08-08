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
#include "timer.h"
#include "memory"
#include "fstream"

#define MAX_EPOLL_EVENTS 20
#define TIMESTEP 5
//#define DEBUG_
/*
创建端口，监听连接
    |
创建内核事件表——使用epoll系列函数，使用ET
    |
*/

int pipe_fd[2];
std::string mysql_username = "zsz";
std::string mysql_passwd = "123456zx";
std::string mysql_database = "zsz_data";
std::string mysql_max_connect_numbers = "10";
void configParser();
int main(int argc, char* argv[]){
    if(argc < 3){
        printf("need filename ip-address port\n");
        return 1;
    }
    configParser();
    //初始化log
    LOG_INIT("logData", "log", 10);
    //ip地址
    char *ip = argv[1];
    //端口
    int port = atoi(argv[2]);
    //创建socket,ipv4.tcp
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        LOG_ERROR("Create socket error %d", errno);
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
        LOG_ERROR("Socket bind error %d", ret);
        LOG_CLOSE();
        return 1;
    }
    //监听
    ret = listen(listenfd, 5);
    if(ret == -1){
        LOG_ERROR("Listen error %d", ret);
        LOG_CLOSE();
        return 1;
    }
    {
        struct linger tmp = {1, 1};
        setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    //创建epoll内核事件表
    int m_epollfd = epoll_create(10);
    //注册sockfd的读事件，也就是监听
    struct epoll_event socketfd_read;
    socketfd_read.events = EPOLLIN;
    socketfd_read.data.fd = listenfd;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, listenfd, &socketfd_read);
    http::setnonblocking(listenfd);

    /*连接数据库->创建数据库连接池->创建线程池*/
    //服务器主循环，持续等待epollfd的事件
    struct epoll_event events[MAX_EPOLL_EVENTS];
    //创建数据库池
    connection_pool::GetInstance()->init("localhost", 3306, mysql_username, mysql_passwd, mysql_database, atoi(mysql_max_connect_numbers.c_str()));
    //创建线程池
    threadPool<http> m_threadPool(connection_pool::GetInstance(), 4, 20);
    //存储连接过来的sockfd和对应的http
    std::map<int, std::shared_ptr<http>> m_data;
    

    //初始化管道
    assert(socketpair(PF_UNIX, SOCK_STREAM, 0, pipe_fd) != -1);
    http::setnonblocking(pipe_fd[0]);
    http::setnonblocking(pipe_fd[1]); 
    //注册管道的读事件
    struct epoll_event pipe_read;
    pipe_read.events = EPOLLIN | EPOLLET;
    pipe_read.data.fd = pipe_fd[0];
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, pipe_fd[0], &pipe_read);
    //初始化定时时间
    timeList m_timerList(TIMESTEP, pipe_fd);

    while(true){
        int nums = epoll_wait(m_epollfd, events, MAX_EPOLL_EVENTS, -1);
       
        if((nums < 0) && (errno != EINTR)){
            LOG_ERROR("epoll wait error: %d", errno);
            LOG_CLOSE();
            return 1;
        }
        
        for(int i = 0; i < nums; ++i){
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
                        LOG_ERROR("add epoll error,client[%s]", inet_ntoa(addr.sin_addr));
                        continue;
                    }
                    m_data[new_con] = temp_http;
                    
                    #ifdef DEBUG_
                    printf("accept new client[%s]\n", inet_ntoa(addr.sin_addr));
                    #endif                    
                    LOG_INFO("accept new client[%s]", inet_ntoa(addr.sin_addr));
                } else {
                    m_data[new_con]->clearData();
                    m_data[new_con]->modfd(m_epollfd, new_con, EPOLLIN);
                }
                //添加定时器
                time_t now;
                time(&now);
                auto timer = std::make_shared<std::vector<long>>(2);
                (*timer.get())[0] = new_con;
                (*timer.get())[1] = now;
                m_timerList.add(timer);
                m_data[new_con]->m_timer = timer;
                
                LOG_INFO("Connect socket[%d], clients[%s]", new_con, inet_ntoa(addr.sin_addr));
                #ifdef DEBUG_
                    printf("Connect socket[%d], clients[%s]\n", new_con, inet_ntoa(addr.sin_addr));
                #endif 

            } 
            //信号
            else if((events[i].data.fd == pipe_fd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                int n = recv(pipe_fd[0], signals, 1024, 0);
                
                if(n <= 0) {
                    continue;
                }
                for(int i = 0; i < n; ++i){
                    switch(signals[i]) {
                        case SIGALRM:
                        {
                            LOG_INFO("SIGALRM");
                            auto sockfds = m_timerList.tick();
                            for(auto sfd : sockfds){
                                close(sfd);
                                epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sfd, 0);
                                //关闭socket
                                LOG_INFO("Close socket, client[%s]", inet_ntoa(m_data[sfd]->get_addr().sin_addr));
                                m_data.erase(sfd);
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
                
                alarm(TIMESTEP);
            }
            //读事件
            else if(events[i].events & EPOLLIN){
                
                int m_fd = events[i].data.fd;
                m_timerList.adjust(m_data[m_fd]->m_timer);
                READ_STAT r_stat = m_data[m_fd]->read_once();
                switch (r_stat)
                {
                //读取的数据有误，直接丢弃，重新触发读事件
                case READ_STAT::BAD:
                {
                    LOG_INFO("Data error, client[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
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
                    //删除定时事件
                    m_timerList.delete_timer(m_data[m_fd]->m_timer);
                    LOG_INFO("Client close socket [%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
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
                    LOG_INFO("Add workList error. client[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
                #ifdef DEBUG_
                    printf("Add workList error.\n");
                #endif
                    //清除数据，重新注册读事件
                    m_data[m_fd]->clearData();
                    http::modfd(m_epollfd, m_fd, EPOLLIN);
                    continue;
                }
                LOG_INFO("deal with client[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
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
                m_timerList.adjust(m_data[m_fd]->m_timer);
                //write_once == false代表要关闭socket
                LOG_INFO("write to client[%s], url[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr), m_data[m_fd]->get_url().c_str());
                if(!m_data[m_fd]->write_once()){
                    LOG_INFO("Close socket, client[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
                    close(m_fd);
                    m_timerList.delete_timer(m_data[m_fd]->m_timer);
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_fd, 0);
                    m_data.erase(m_fd);
                } 
                
            }
            
        }
    }
}

void configParser(){
    std::ifstream file_in("config/config");
    if(!file_in) return;
    std::string line;
    std::map<std::string, std::string> data;
    while(getline(file_in, line)){
        size_t index = line.find(':',0);
        size_t start = line.find_first_not_of(' ');
        size_t end = line.find_last_not_of(' ', index);
        std::string a_ = line.substr(start, end - start + 1);
        start = line.find_first_not_of(' ', index + 1);
        end = line.find_last_not_of(' ');
        std::string b_ = line.substr(start, end - start + 1);
        data[a_] = b_;
    }
    if(data.find("mysql_username") != data.end()){
        mysql_username = data["mysql_username"];
    }
    if(data.find("passwd") != data.end()){
        mysql_passwd = data["passwd"];
    }
    if(data.find("database") != data.end()){
        mysql_database = data["database"];
    }
    if(data.find("mysql_max_connect_numbers") != data.end()){
        mysql_max_connect_numbers = data["mysql_max_connect_numbers"];
    }
    return;
}