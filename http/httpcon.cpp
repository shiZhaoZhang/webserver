#include "httpconn.h"

//初始化
bool http::init(int epollfd, int sockfd){
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    //注册失败
    if(epollAdd(m_epollfd, m_sockfd) == -1){
        return false;
    }
    return true;
}


//读取sockfd中的信息
READ_STAT http::read_once(){
    std::string buffer(BUFFER_SIZE, '\0');
    //由于采用的是ET模式，所以需要一次性读完
    ssize_t len = 0;
    while(true){
        ssize_t ret = recv(m_sockfd, &*buffer.begin() + len, BUFFER_SIZE - len, 0);
        if(ret < 0){
            //对于非阻塞IO，下边条件成立代表数据全部读完，跳出循环
            if((errno == EAGAIN)||(errno == EWOULDBLOCK)){
                break;
            }
            //否则读取失败
            return READ_STAT::BAD;
        } else if(ret == 0){
            //判断是不是buffer的剩余空间不够了，是的话先把结果存在message，把buffer清空，继续读
            if(len == BUFFER_SIZE){
                m_message += buffer;
                buffer = std::string(BUFFER_SIZE, '\0');
                len = 0;
                continue;
            }
            return READ_STAT::CLOSE;
        } else {
            len += ret;
        }
    }
    m_message += buffer.substr(0, len);
    return READ_STAT::OK;
}


/****************************************************************\
 *                           辅助函数                             *
\****************************************************************/



//成功返回旧值，失败返回-1。
int http::setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    int err = fcntl(fd, F_SETFL, new_option);
    return err == -1? -1 : old_option;
}

//成功返回0，失败返回-1
int http::epollAdd(int epollfd, int sockfd){
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    event.data.fd = sockfd;
    //把sockfd设置为非阻塞
    if(setnonblocking(sockfd) == -1){
        return -1;
    }
    //注册
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1){
        return -1;
    }
    return 0;
}

//成功返回0，失败返回-1
int http::resetOneshot(int epollfd, int sockfd){
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    event.data.fd = sockfd;
    //修改
    if(epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event) == -1){
        return -1;
    }
    return 0;
}

