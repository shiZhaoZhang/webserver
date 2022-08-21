#ifndef TIMER_H
#define TIMER_H


//使用timerfd_*系列函数来制作定时器
#include "unistd.h"
#include "signal.h"
#include "time.h"
#include "string.h"
#include "sys/socket.h"
#include "assert.h"
#include <sys/timerfd.h>
#include "list"
#include "memory"
#include "vector"
#include "rlog.h"

class timefdList{
public:
    //使用统一事件源，fd用来设置timerfd_create创建的定时器对象的文件描述符，在主函数中用于注册epoll事件
    timefdList(long timeStep, int &fd);
    ~timefdList(){}
    //添加事件，组成{socket, time}
    void add(std::shared_ptr<std::vector<long>> data);
    void adjust(std::shared_ptr<std::vector<long>> data, int n); 
    void delete_timer(std::shared_ptr<std::vector<long>> data);

    //主循环检测到定时事件，使用tick来移除长时间未操作的socket，并返回删除的socket列表
    std::vector<long> tick();
    
private:
    long m_timeStep; //时间间隔
    std::list<std::shared_ptr<std::vector<long>>> m_list; //存储时间和socket
    int m_fd;

};



#endif