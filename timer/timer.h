#ifndef TIMER_H
#define TIMER_H
#include "unistd.h"
#include "signal.h"
#include "time.h"
#include "string.h"
#include "sys/socket.h"
#include "assert.h"

#include "list"
#include "memory"
#include "vector"

class timeList{
public:
    //使用统一事件源，管道用来传递1端写，0端读
    timeList(long timeStep, int *fd = nullptr);
    ~timeList(){}
    void start();
    //添加事件，组成{socket, time}
    void add(std::shared_ptr<std::vector<long>> data);
    void adjust(std::shared_ptr<std::vector<long>> data); 
    void delete_timer(std::shared_ptr<std::vector<long>> data);

    //主循环检测到定时事件，使用tick来移除长时间未操作的socket，并返回删除的socket列表
    std::vector<long> tick();
    
    static void addsig(int sig);
private:
    long m_timeStep; //时间间隔
    std::list<std::shared_ptr<std::vector<long>>> m_list; //存储时间和socket
    static int *m_fd;
private:
    static void sig_handler(int sig);
};
int *timeList::m_fd = nullptr;

timeList::timeList(long timeStep, int *fd){
    //输入必须大于0
    m_timeStep = timeStep > 0 ? timeStep : 1;    
    m_fd = fd;
    //设置信号处理函数
    addsig(SIGALRM);
    //开始记时
    alarm(m_timeStep);
}

//开始记时
void timeList::start(){
    alarm(m_timeStep);
}

//增加
void timeList::add(std::shared_ptr<std::vector<long>> data){
    if(data == nullptr) return;
    auto iter = m_list.begin();
    for(; iter != m_list.end(); ++iter){
        if((*(*iter).get())[1] > (*data.get())[1]) break;
    }
    m_list.insert(iter, data);
}

//删除
void timeList::delete_timer(std::shared_ptr<std::vector<long>> data){
    auto iter = m_list.begin();
    for(; iter != m_list.end(); ++iter){
        if(*iter == data) break;
    }
    m_list.erase(iter);
}
//修改=删除+添加
void timeList::adjust(std::shared_ptr<std::vector<long>> data){
    delete_timer(data);
    time_t now;
    time(&now);
    (*data.get())[1] = now + m_timeStep * 3;
    add(data);
}

//执行事件
std::vector<long> timeList::tick(){
    std::vector<long> res;
    time_t now;
    time(&now);
    auto iter = m_list.begin();
    for(; iter != m_list.end(); ++iter){
        if((*(*iter).get())[1] > now) break;
        res.push_back((*(*iter).get())[0]);
    }
    m_list.erase(m_list.begin(), iter);
    
    return res;
}

/******************************************************\
 *                    辅助函数                          *
\******************************************************/
void timeList::sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(m_fd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void timeList::addsig(int sig){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = timeList::sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

#endif