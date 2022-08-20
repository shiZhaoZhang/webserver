#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "rlog.h"
#include <memory>
class mmapFile;

//构筑响应报文
class http_response{
public:
    http_response():message_length(0){}
    ~http_response(){}
public:
    //根据状态码构造状态栏，消息报文和空行，第一个调用
    void base_request(const std::string code);
    //添加Content-Type
    void add_ContentType(const std::string contentType);
    //添加Server
    void add_Server(const std::string serverName);
    //添加Content-Lenght实体字段
    void add_ContentLength(const long int length);
    //首部字段结束
    inline void end_response_message_head(){
        response_message_head += "\r\n";
    }
    //添加实体
    //实体是文件
    void add_file(const std::string &fileName);
    //实体是可以用字符串描述的内容
    void add_message(const std::string &message);
public:
    std::string response_message_head;
    //文件
    std::shared_ptr<mmapFile> response_message_body_file;
    //字符串信息
    std::string response_message_body_message; 
    long int message_length;
};

//传入文件名，用mmap映射文件，使用RAII管理
class mmapFile{
public:
    mmapFile():m_file(nullptr), m_fd(-1){}
    mmapFile(const std::string fileName);
    ~mmapFile();
    //获取mmap返回的内存指针
    void* getMemory(){return m_file;}
    //获取文件的信息
    struct stat get_fileStat(){ return m_file_stat;};
    //获取最后一次出错的信息
    std::string get_fileName(){ return m_filename;}
    //获取文件长度
    long int get_fileLength();
private:
    int m_fd;
    struct stat m_file_stat;
    void *m_file;
    std::string m_filename;
};
#endif