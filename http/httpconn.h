//http连接对象
#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string>

#define BUFFER_SIZE 4096

//read_once函数会出现三种结果，读取成功，读取失败，对方socket关闭
enum class READ_STAT{OK, BAD, CLOSE};

class http{
public:
    http() = default;
    //不允许拷贝
    http(const http&) = delete;
    //不允许赋值
    http& operator=(const http&) = delete;
    ~http(){}

    //初始化
    bool init(int epollfd, int sockfd);
    //读取m_sockfd中的信息
    READ_STAT read_once();
    //分析message结果

//下边三个是辅助函数
    //把文件描述符设置为非阻塞
    static int setnonblocking(int fd);
    //注册socket事件
    static int epollAdd(int epollfd, int sockfd);
    //对于EPOLLONESHOT事件需要重新注册
    int http::resetOneshot(int epollfd, int fd);
private:
    int m_epollfd;
    int m_sockfd;

    std::string m_message;
    
};
#endif
