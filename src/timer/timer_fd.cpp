#include "timer_fd.h"


timefdList::timefdList(long timeStep, int &fd){
    //输入必须大于0
    m_timeStep = timeStep > 0 ? timeStep : 1;    
    //初始化计时器，设置为非阻塞
    m_fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    fd = m_fd;
    if(m_fd < 0){
        ERROR("Create timerfd error");
        return;
    }
    //设置计时器
    time_t now;
    time(&now);
    struct itimerspec m_timer;
    m_timer.it_value.tv_sec = now + m_timeStep;
    m_timer.it_value.tv_nsec = 0;
    m_timer.it_interval.tv_sec = m_timeStep;
    m_timer.it_interval.tv_nsec = 0;
    timerfd_settime(m_fd, 1, &m_timer, nullptr);
}


//增加
void timefdList::add(std::shared_ptr<std::vector<long>> data){
    if(data == nullptr) return;
    auto iter = m_list.begin();
    for(; iter != m_list.end(); ++iter){
        if((*(*iter).get())[1] > (*data.get())[1]) break;
    }
    m_list.insert(iter, data);
}

//删除
void timefdList::delete_timer(std::shared_ptr<std::vector<long>> data){
    auto iter = m_list.begin();
    for(; iter != m_list.end(); ++iter){
        if(*iter == data) break;
    }
    m_list.erase(iter);
}
//修改=删除+添加
void timefdList::adjust(std::shared_ptr<std::vector<long>> data, int n){
    delete_timer(data);
    time_t now;
    time(&now);
    (*data.get())[1] = now + m_timeStep * n;
    add(data);
}

//执行事件
std::vector<long> timefdList::tick(){
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