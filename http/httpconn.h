//http连接对象
#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string>
#include "httpRequestParser.h"
#include <mysql/mysql.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include "memory"

#define BUFFER_SIZE 4096

//read_once函数会出现三种结果，读取成功，读取失败，对方socket关闭
enum class READ_STAT{OK, BAD, CLOSE};

class http{
public:
    http(struct sockaddr_in addr)
    :m_addr(addr){

    }
    /*
    //不允许拷贝
    http(const http&) = delete;
    //不允许赋值
    http& operator=(const http&) = delete;
    */
    ~http(){}

    //初始化
    bool init(int epollfd, int sockfd);
    //清空
    void clearData();
    //读取m_sockfd中的信息
    READ_STAT read_once();
    //主线程写,true代表写完保持连接，false代表关闭
    bool write_once();

    //返回所连接的socket的地址
    struct sockaddr_in get_addr(){
        return m_addr;
    }

//下边三个是辅助函数
    //把文件描述符设置为非阻塞
    static int setnonblocking(int fd);
    //注册socket事件
    static int epollAdd(int epollfd, int sockfd, int ev);
    //对于EPOLLONESHOT事件需要重新注册
    static int modfd(int epollfd, int fd, int ev);

    inline std::string get_url(){
        return request_parase.get_URL();
    }
private:
    int m_epollfd;
    int m_sockfd;
    struct sockaddr_in m_addr;

    http_request request_parase;
    
    //接收到的请求报文的内容
    std::string m_message;
    
    //响应报文的：状态栏，消息报文和空行
    std::string respond_message;
    //respond_body中的是html所在的位置
    std::string respond_body;
    //根据respond_message和respond_body构建iovec
    struct iovec m_iovec[2]; //用于wirtev，长度为2
    
public:
    MYSQL *mysql;
    void process();
    std::shared_ptr<std::vector<long>> m_timer;
private:
    
    //构造响应报文
    //BAD_REQUEST的响应
    void bad_request();
    //INTERNAL_ERROR的响应
    void internal_error();
    //GET_REQUEST响应
    void get_request();
    //根据状态码构造状态栏，消息报文和空行
    void base_request(std::string code, int length = 0){
        respond_message.clear();
        respond_body.clear();
        //状态栏
        respond_message = "HTTP/1.1 " + code + "\r\n";
        //消息报文
        {
            time_t rawTime;
            struct tm* timeInfo;
            char szTemp[36]={'\0'};
            time(&rawTime);
            timeInfo = gmtime(&rawTime);
            strftime(szTemp,sizeof(szTemp),"Data: %a, %d %b %Y %H:%M:%S GMT",timeInfo);
            respond_message += szTemp;
        }
        respond_message += "\r\n";
        //respond_message += "Content-Type: text/html; charset=UTF-8\r\n";
       // respond_message += "Connection:close\r\n";
        respond_message += "\r\n";
        
    }
    //添加新用户
    bool InsertUser(const char* user_name, const char *passwd);
    //查看用户是否存在
    std::string ExistUser(const char*user_name);
};
#endif
