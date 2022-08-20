#ifndef LOG_H
#define LOG_H
#include "blockQueue/blockQueueFactory.h"
#include "utc_timer.h"
#include "string"
#include "fstream"
#include "memory"
#include "iostream"
//日志级别
enum class LOG {DEBUG, INFO, WARN, ERROR};

class Log{
public:
    static Log* getInstance(){
        /*两层nullptr判断是因为：
         + 如果有多个线程同时获取instance，而且都过了第一关，那么先获取到锁的线程负责初始化。
         + 如果把加锁放在一开始，那么每次获取实例都会进行一次加解锁，会消耗大量内核资源。
        */
        static locker m_locker;
        if(instance == nullptr){
            MutexLockGuard lock(m_locker);
            if(instance == nullptr){
                instance = new Log();
            }
        }
        return instance;
    }
    bool init(uint64_t maxLineNums, bool asyn = true);
    void close();
    void wirte_log(LOG, const std::string);
    
    static void *pthread_func(void*){
        Log::getInstance()->asyn_write();
    }

    static bool isFileExists_stat(const std::string &);

private:
    Log():isOpen(false){}
    ~Log(){}
    //异步写
    void asyn_write();
    //同步写
    void syn_write(const std::string &);
    //是否要新文件
    void newFile();
private:
    static Log *instance;
    locker m_mutex;
    bool isOpen;    //日志系统是否处于开启状态

    //文件相关
    std::fstream fstrm;                 //文件流
    std::string m_fileName;             //log文件名称
    uint64_t lineNums;                  //当前log文件的行数
    uint64_t m_maxLineNums;             //log文件允许的最大行数，超过则生成另一个文件
    uint64_t m_today;                   //因为按天分类,记录当前时间是那一天     
    uint64_t nfile;                     //当天的第几个log
    //异步相关
    bool m_asyn;                          //是否异步
    std::shared_ptr<IBlockQueue<std::string>> m_queue;  //阻塞队列
    pthread_t m_thread;
    
    //时间相关
    utc_timer m_timer;
};


#define LOG_DEBUG(m) Log::getInstance()->wirte_log(LOG::DEBUG, m);
#define LOG_INFO(m) Log::getInstance()->wirte_log(LOG::INFO, m);
#define LOG_WARN(m) Log::getInstance()->wirte_log(LOG::WARN, m);
#define LOG_ERROR(m) Log::getInstance()->wirte_log(LOG::ERROR, m);

#endif