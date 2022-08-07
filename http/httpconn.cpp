#include "httpconn.h"
#define DEBUG_

//初始化
bool http::init(int epollfd, int sockfd){
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    //注册失败
    if(epollAdd(m_epollfd, m_sockfd, EPOLLIN) == -1){
        return false;
    }
    return true;
}

//清空
void http::clearData(){
    m_message.clear();
    request_parase.clear();
    respond_message.clear();
    respond_body.clear();
    if(m_iovec[1].iov_base){
        munmap(m_iovec[1].iov_base, m_iovec[1].iov_len);
    }
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

//主线程：往socket写
bool http::write_once(){
    size_t len = m_iovec[0].iov_len + m_iovec[1].iov_len;
    while(true){
        ssize_t temp_len = writev(m_sockfd, m_iovec, 2);
        if(temp_len < 0){
            //出现阻塞，重新触发写事件，下次写
            if(errno == EAGAIN){
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            } 
            //出现问题
            clearData();
            return false;
        }
        len -= (size_t)temp_len;
        //数据写完
        if(len == 0){
            auto iter = request_parase.get_head_params().find("Connection");
            //长连接
            if(iter == request_parase.get_head_params().end() || iter->second == "keep-alive"){
                clearData();
                //继续监听m_sockfd套接字，重新触发读事件，移除写事件
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
            //短连接
            clearData();
            return false;
        }

        //没有传完，修改传输的起始位置
        if(temp_len <= m_iovec[0].iov_len){
            m_iovec[0].iov_base = (void*)((char*)m_iovec[0].iov_base + temp_len);
            m_iovec[0].iov_len -= temp_len;
        } else {
            m_iovec[1].iov_base = (void*)((char*)m_iovec[1].iov_base + temp_len - m_iovec[0].iov_len);
            m_iovec[1].iov_len -= temp_len - m_iovec[0].iov_len;

            m_iovec[0].iov_base = (void*)((char*)m_iovec[0].iov_base + m_iovec[0].iov_len);
            m_iovec[0].iov_len = 0;
        }

    }
}

void http::process(){
    //解析报文
    HTTP_CODE code = this->request_parase.parser(this->m_message);
    //根据报文不同的解析结果返回不同的响应报文。
    switch (code)
    {
    case HTTP_CODE::OPEN_REQUEST:
        return;
        break;
    case HTTP_CODE::BAD_REQUEST:
        printf("message:\n%s", m_message.c_str());
        printf("error : %s\n", request_parase.get_error_message().c_str());
        bad_request();
        break;
    case HTTP_CODE::INTERNAL_ERROR:
        internal_error();
        break;
    case HTTP_CODE::GET_REQUEST:
        get_request();
        break;
    default:
        break;
    }
#ifdef DEBUG_
    printf("message:%s", respond_message.c_str());
    printf("body:%s\n", respond_body.c_str());
#endif
    //判断响应报文的消息体是否为空
    m_iovec[0].iov_base = (void*)(respond_message.c_str());
    m_iovec[0].iov_len = respond_message.size();
    if(respond_body.empty()){    
        m_iovec[1].iov_base = nullptr;
        m_iovec[1].iov_len = 0;
    } else {
        std::string filename = "root/" + respond_body;
        int fd = open(filename.c_str(), O_RDONLY);
        struct stat *file_stat;
        stat(filename.c_str(), file_stat);
        m_iovec[1].iov_base = mmap(nullptr, file_stat->st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        m_iovec[1].iov_len = file_stat->st_size;
    }
    //触发epollfd写事件
    if(modfd(m_epollfd, m_sockfd, EPOLLOUT) == -1){
        return;
    }
    #ifdef DEBUG_
    printf("process : mod fd EPOLLOUT\n");
    #endif
}

void http::bad_request(){
    base_request("400 Bad Request");
    respond_body.clear();
    return;
}
void http::internal_error(){
    base_request("500 Internal Server Error");
    respond_body.clear();
    return;
}
void http::get_request(){
    std::string m_url = request_parase.get_URL();
    if(m_url == "/" && request_parase.get_method() == "GET"){
        base_request("200 OK");
        respond_body = "judge.html";
        return;
    } else {
        base_request("404 Not Found");
        respond_body.clear();
    }
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
int http::epollAdd(int epollfd, int sockfd, int ev){
    struct epoll_event event;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
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
int http::modfd(int epollfd, int sockfd, int ev){
    struct epoll_event event;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    event.data.fd = sockfd;
    //修改
    if(epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event) == -1){
        return -1;
    }
    return 0;
}
