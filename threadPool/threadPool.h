#include <queue>
#include <pthread.h>
#include <vector>
#include "locker.h"
#include <memory>
#include <iostream>
//要求T类型一定要有一个process()函数

template<typename T>
class threadPool{
public:
    threadPool(int num = 4, int maxListNums = 100);
    ~threadPool();
    bool append(std::shared_ptr<T>);
private:
    static void* work(void*);
    void run();
private:
    //工作队列
    std::queue<std::shared_ptr<T>> workList;
    //线程ID保存
    std::vector<pthread_t> m_threadList;
    //工作队列最大数目
    int MAX_LIST_NUMS;
    //创建线程数目
    int m_thread_nums;
    //锁
    locker m_mutex;
    //信号量
    sem m_sem;
    //线程池是否开启
    bool isOpen;
};
template<typename T>
void *threadPool<T>::work(void* t){
    threadPool *request = static_cast<threadPool*>(t);
    request->run();
    pthread_exit(nullptr);
}


//构造函数
template<typename T>
threadPool<T>::threadPool(int num, int maxListNums)
    :m_thread_nums(num), MAX_LIST_NUMS(maxListNums), m_threadList(num), isOpen(true)
{
    for(int i = 0; i != m_thread_nums; ++i){
        int err = pthread_create(&m_threadList[i], nullptr, work, this);
        if(err != 0){
            std::cerr <<  "[" << __FILE__ << "]: " << __LINE__ << " "<<  __FUNCTION__ << ": pthread_create error " << err << std::endl;
            exit(0);
        }
    }
}
//析构函数
template<typename T>
threadPool<T>::~threadPool(){
    //关闭线程循环
    isOpen = false;
    //回收线程
    for(int i = 0; i != m_thread_nums; ++i){
        pthread_join(m_threadList[i], nullptr);
    }
}
//添加到队列
template<typename T>
bool threadPool<T>::append(std::shared_ptr<T> request){
    MutexLockGuard m_locker(m_mutex);
    if(!isOpen || workList.size() >= MAX_LIST_NUMS){
        return false;
    }
    workList.push(request);
    //信号量+1
    m_sem.post();
    return true;
}
//取出并运行
template<typename T>
void threadPool<T>::run(){
    std::shared_ptr<T> request;
    //线程关闭且请求队列空
    while(isOpen && !workList.empty())
    {
        m_sem.wait();
        {
            MutexLockGuard m_locker(m_mutex);
            //如果被其他线程抢先用了
            if(workList.empty()) continue;
            request = workList.front();
            workList.pop();
        }
        request->process();
    }
    
}