#ifndef WEBSERVER
#define WEBSERVER
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
#include "timer_fd.h"
#include "memory"
#include "fstream"
#include <functional>

#define TIMESTEP 1
#define MAX_EPOLL_EVENTS 200

class webserver{
public:
    webserver();
    ~webserver(){
        LOG_CLOSE();
    }
    //删除拷贝构造
    webserver(const webserver&) = delete;
    //禁止复制
    void operator=(const webserver &) = delete;
    
    bool webListen(std::string ip, int port);
    //输入一个连接几秒之后无行为会关闭
    void run(int);
    //void addStaticSource(const std::string url, const std::string fileName);
    //void addGet(const std::string, void (*func) (http_request&, http_response&, MYSQL*));
    //void addPost(const std::string, void (*func) (http_request&, http_response&, MYSQL*));
    void addStaticSource(const std::string url, const std::string fileName);
void addGet(const std::string, void (*func) (http_request&, http_response&, MYSQL*));
void addPost(const std::string, void (*func) (http_request&, http_response&, MYSQL*));
private:
    //解析config文件
    std::string mysql_ip;
    int mysql_port;
    void configParser();
    std::string mysql_username;
    std::string mysql_passwd;
    std::string mysql_database;
    int mysql_max_connect_numbers;
    int thread_pool_max_list;
    int thread_pool_nums;

    //ip地址和监听端口
    std::string m_ip;
    int m_port;
    //监听的文件描述符
    int m_listenfd;

    //内核事件表
    int m_epollfd;

    //线程池
    std::shared_ptr<threadPool<http>> m_threadPool;

    //存储连接过来的sockfd和对应的http
    std::map<int, std::shared_ptr<http>> m_data;

    //定时器相关
    int m_alarm_;
    timefdList m_timerfdList;
private:
/*
    辅助函数
*/
    //设置文件描述符为非阻塞，成功返回旧值，失败返回-1。
    int setnonblocking(int fd){
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        int err = fcntl(fd, F_SETFL, new_option);
        return err == -1? -1 : old_option;
    }
};

#endif