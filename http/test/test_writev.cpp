//使用writev分散写，写头部文件和给定的目标文件
#include "create_sockfd.h"
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "string"

bool fileCheck(const char *filename, struct stat *file_stat, char **fileBuf) {
    //小于0代表目标文件不存在。
    if(stat(filename, file_stat) < 0){
        printf("filename is not exist.\n");
        return false;
    } else {
        //判断目标文件是不是目录
        if(S_ISDIR(file_stat->st_mode)) {
            printf("filename is dir.\n");
            return false;
        } else if (!(file_stat->st_mode & S_IROTH)) { //有无读取权限
            printf("filename meiyouquanxian.\n");
            return false;
        } 
    }
    if(fileBuf == nullptr){
        return true;
    }
    int fd = open(filename, O_RDONLY);
    *fileBuf = new char [file_stat->st_size + 1];
    memset(*fileBuf, '\0', file_stat->st_size + 1);
    //读取失败
    if(read(fd, *fileBuf, file_stat->st_size) < 0){
        delete [] *fileBuf;
        printf("filename read error.\n");
        return false;
    }
    return true;
}


//定义缓冲区大小
#define BUFFER_SIZE 1024

//定义两种HTTP状态码和状态信息
static const char *status_line[2] = { "200 OK", "500 Internal server error"};

int main(int argc, char *argv[]){
   if(argc <= 3){
       printf("usage: %s ip_address port_number filename\n", basename(argv[0]));
       return 1;
   }


   createSockfd sockfd(argv[1], atoi(argv[2]));

   assert(sockfd.bindSockfd() != -1);

   assert(sockfd.listenfd(5) != -1);

   sockfd.acceptfd();

   if(sockfd.connfd < 0) {
       printf("Accept error: %d\n", errno);
       return 1;
   }
       //保存头部信息
       char header_buf[BUFFER_SIZE];
       memset(header_buf, '\0', BUFFER_SIZE);
       //记录header用了多少空间
       int len = 0;
      

       //存放文件信息
       struct stat file_stat;
        //用于存放目标文件的缓存
       char *filename = argv[3];
       if(!fileCheck(filename, &file_stat, nullptr)){
            return 1;
       }
        int fd = open(filename, O_RDONLY);
       void *fileBuf = mmap(nullptr, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
           
        int ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
        len += ret;
        ret = snprintf(header_buf + len, BUFFER_SIZE - 1, "Content-Length: %d\r\n", int(file_stat.st_size));
        len += ret;
        ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");
        len += ret;
        struct iovec iv[2];
        std::string s = header_buf;
        iv[0].iov_base = (void*)(s.c_str());
        iv[0].iov_len = s.size();
        printf("%s", s.c_str());
        iv[1].iov_base = (void*)((char*)fileBuf + 5);
        iv[1].iov_len = file_stat.st_size - 5;

        ssize_t send_n = writev(sockfd.connfd, iv, 2);
        printf("header_buf length %d bytes, file %d bytes, send %d byte \n", iv[0].iov_len, file_stat.st_size, send_n);
           
       
       close(fd);
   
   return 0;
} 
