#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H
#include "../../lock/locker.h"
#include "queue"
#include "IBlockQueue.h"
template<typename T>
class block_queue : public IBlockQueue<T>{
public:

    block_queue(int nums):MAX_SIZE(nums){};
    block_queue():block_queue(1000){};
    ~block_queue(){};

    bool push(T);
    bool pop(T&);
    bool pop(T&, struct timespec);

    int setMaxSize(const int n){
        MAX_SIZE = n;
    }
    int size();
    int max_size();
private:
    std::queue<T> m_queue;
    int MAX_SIZE;
    locker m_mutex;
    cond m_cond;
};
template<typename T>
bool block_queue<T>::push(const T element){
    MutexLockGuard lock(m_mutex);
    if(m_queue.size() == MAX_SIZE){
        m_cond.broadcast();
        return false;
    }
    m_queue.push(element);
    m_cond.broadcast();
    return true;
}

template<typename T>
bool block_queue<T>::pop(T &element){
    MutexLockGuard lock(m_mutex);
    while(m_queue.size() == 0){
        if(!m_cond.wait(m_mutex.get()))
            return false;
    }
    element = m_queue.front();
    m_queue.pop();
    return true;
}

template<typename T>
bool block_queue<T>::pop(T &element, struct timespec t){
    MutexLockGuard lock(m_mutex);
    while(m_queue.size() == 0){
        if(!m_cond.timewait(m_mutex.get(), t))
            return false;
    }
    element = m_queue.front();
    m_queue.pop();
    return true;
}

template<typename T>
int block_queue<T>::size(){
    MutexLockGuard lock(m_mutex);
    return m_queue.size();
}
template<typename T>
int block_queue<T>::max_size(){
    MutexLockGuard lock(m_mutex);
    return MAX_SIZE;
}
#endif
