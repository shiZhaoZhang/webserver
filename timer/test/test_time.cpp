//测试统一事件源
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include "assert.h"
#include "signal.h"
#include "fcntl.h"

int pipe_fd[2];
void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(pipe_fd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
//成功返回旧值，失败返回-1。
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    int err = fcntl(fd, F_SETFL, new_option);
    return err == -1? -1 : old_option;
}
int main(){

    //创建epoll内核事件表
    int m_epollfd = epoll_create(10);
     //初始化管道
    assert(socketpair(PF_UNIX, SOCK_STREAM, 0, pipe_fd) != -1);
    setnonblocking(pipe_fd[1]);

    struct epoll_event pipe_read;
    pipe_read.events = EPOLLIN | EPOLLET;
    pipe_read.data.fd = pipe_fd[0];
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, pipe_fd[0], &pipe_read);
    setnonblocking(pipe_fd[0]);
    addsig(SIGALRM);
    alarm(5);
    struct epoll_event events[10];
    while(true){
        int nums = epoll_wait(m_epollfd, events, 10, -1);
        printf("nums = %d\n", nums);
        if((nums < 0) && (errno != EINTR)){
            printf("epoll wait error: %d\n", errno);
            
            return 1;
        }
        for(int i = 0; i < nums; ++i){
            if((events[i].data.fd == pipe_fd[0]) && (events[i].events & EPOLLIN)) {
                int sig;
                char signals[1024];
                int n = recv(pipe_fd[0], signals, 1024, 0);
                printf("n = %d\ntick()\n", n);
                alarm(5);
            }
            else {
                printf("other\n");
            }
        }
        
    }
}