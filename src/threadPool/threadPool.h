#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>
#include <rlog.h>
#include "connect_pool.h"



//要求T类型一定要有一个process()函数

template<typename T>
class threadPool:public std::enable_shared_from_this<threadPool<T>>{
public:
    threadPool():isInit(false), isopen(false){};
    ~threadPool();
    void init(connection_pool* conn_pool, int num = 4, int maxListNums = 100);
    bool append(std::shared_ptr<T>);
    //主动关闭以及提供外界感知线程池状态
    void close(){
        std::lock_guard<std::mutex> locker(m_mutex);
        isopen = false;
    }
    bool isOpen() const {return isopen; }
private:
    static void work(std::shared_ptr<threadPool<T>>);
    void run();

private:
    //工作队列
    std::queue<std::shared_ptr<T>> workList;
    //线程保存用于join
    std::vector<std::shared_ptr<std::thread>> m_threadList;
    //工作队列最大数目d
    int MAX_LIST_NUMS;
    //创建线程数目
    int m_thread_nums;
    //互斥锁
    std::mutex m_mutex;
    //条件变量
    std::condition_variable m_con;
    //线程池是否开启
    bool isopen;
    //数据库池
    connection_pool *m_connPool;
    //防止多次调用init函数
    bool isInit;
};

template<typename T>
void threadPool<T>::work(std::shared_ptr<threadPool<T>> request){
    request->run();
}


//初始化
template<typename T> 
void threadPool<T>::init(connection_pool* conn_pool, int num, int maxListNums){
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if(isInit){
            return;
        }
        isInit = true;
    }
    //初始化参数
    m_thread_nums = num;
    MAX_LIST_NUMS = maxListNums;
    m_threadList = std::vector<std::shared_ptr<std::thread>>(num);
    isopen = true;
    m_connPool = conn_pool;

    for(int i = 0; i != m_thread_nums; ++i){
        std::shared_ptr<std::thread> temp;
        //增加异常捕获，如果当前线程创建失败，则不需要添加进m_threadList
        try{
            std::shared_ptr<std::thread> t = std::make_shared<std::thread>(work, threadPool<T>::shared_from_this());
            temp.swap(t);
        }
        catch(const std::system_error &e){
            //报告错误
            LOG_FATAL("Create thread eroror : %s", e.what());
            continue;
        }
        this->m_threadList.push_back(temp);

    }
    LOG_INFO("Init thread success");
}
//析构函数
template<typename T>
threadPool<T>::~threadPool(){
    //关闭线程循环
    isopen = false;
    //回收线程
    for(auto &t : this->m_threadList){
        t->join();
    }
}
//添加到队列
template<typename T>
bool threadPool<T>::append(std::shared_ptr<T> request){
    std::lock_guard<std::mutex> m_locker(m_mutex);
    if(!isopen || workList.size() >= MAX_LIST_NUMS){
        return false;
    }
    workList.push(request);
    //信号量+1
    m_con.notify_all();
    return true;
}
//取出并运行
template<typename T>
void threadPool<T>::run(){
    LOG_INFO("Running...");
    //线程关闭且请求队列空才退出
    while(isopen || !workList.empty())
    {
        std::shared_ptr<T> request;
        {
            std::unique_lock<std::mutex> lck(m_mutex);
            m_con.wait(lck, [this](){return !this->workList.empty();});
            request = workList.front();
            workList.pop();
        }

        if(request == nullptr) continue;
        ConnectRAII mysql_raii(m_connPool);
        request->mysql = mysql_raii.getConn();
        request->process();
    }
    
}
#endif