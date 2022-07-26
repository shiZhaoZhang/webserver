//读取报文内容，主要是想看post发出的报文的具体样式
#include <pthread.h>
#include "create_sockfd.h"
#include "iostream"
#include "string"
using namespace std;

#define BUFFER_SIZE 10
int main(int argc, char* argv[]){
    if(argc <= 2){
        printf("Ussage: %s ip_address port_number.\n", basename(argv[0]));
    }
    createSockfd sockfd(argv[1], atoi(argv[2]));
    assert(sockfd.bindSockfd() == 0);
    assert(sockfd.listenfd() == 0);
    assert(sockfd.acceptfd() != -1);
    if(sockfd.connfd < 0){
        printf("Accept error, errno is: %d.\n", errno);
    } else {
        //char buffer[BUFFER_SIZE];
        //memset(buffer, '\0', BUFFER_SIZE);
        string result;
        string buffer(BUFFER_SIZE, '\0');
        int data_read = 0;
        int read_index = 0;     //当前已经读取了多少字节用户数据
        int checked_index = 0;  //当前已经分析了多少字节用户数据
        int start_line = 0;     //行在buffer中的起始位置
        int all = 0;
        //循环读取客户请求
        //setnonblocking(sockfd.connfd);
        while(1){
            data_read = recv(sockfd.connfd, &*buffer.begin() + read_index, BUFFER_SIZE - read_index, 0);
            //读取失败
            if (data_read == -1){
                if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                    printf("read later\n");
                    break;
                }
                printf("Reading failed\n");
                close(sockfd.connfd);
                break;
            } else if(data_read == 0){  //关闭了socket连接
                if(read_index == BUFFER_SIZE){
                    all += read_index;
                    result += buffer;
                    buffer = string(BUFFER_SIZE, '\0');
                    read_index = 0;
                    continue;
                }
                printf("Remote client has close the connection.\n");
                break;
            }
            read_index += data_read;
            if(read_index > 4 && buffer[read_index - 4] == '\r' &&  buffer[read_index - 3] == '\n' && buffer[read_index - 2] == '\r' &&  buffer[read_index - 1] == '\n'){
                break;
            }
            
        }
        all += read_index;
        cout << result + buffer.substr(0, read_index) << endl << "read_index = " << all << endl;
        close(sockfd.connfd);
    }

    return 0;
}