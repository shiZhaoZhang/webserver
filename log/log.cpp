#include "log.h"
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>

//辅助函数：判断文件是否存在
bool Log::isFileExists_stat(const std::string& name) {
  struct stat buffer;   
  return (stat(name.c_str(), &buffer) == 0); 
}
//初始化log
bool Log::init(uint64_t maxLineNums, bool asyn, int queuesize){
    
    //初始化log文件的名称为zszweblog_year_month_day_n，存储在当前路径的Log文件夹下
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t _sys_acc_sec = tv.tv_sec;
    struct tm cur_tm;
    localtime_r((time_t*)&_sys_acc_sec, &cur_tm);
    char utc_fmt[20];
    snprintf(utc_fmt, 20, "log/zszweblog_%02d_%02d_%02d", cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday);
    //创建文件名
    std::string tempFileName(utc_fmt);//zszweblog_ + year_month_day_n
    //判断该文件名是否存在
    if(isFileExists_stat(tempFileName)){
        //文件存在的话在后边加上序号_n
        int i = 1;
        while(isFileExists_stat(tempFileName + "_" + std::to_string(i))){
            ++i;
        }
        m_fileName = tempFileName + "_" + std::to_string(i);
    } else {
        m_fileName = tempFileName;
    }

    //开启文件流
    fstrm.open(m_fileName, std::ios_base::app);
    if(!fstrm){
        std::cerr << "open " << m_fileName << " error\n";
        exit(1);
    }
    //
    nfile = 0;

    //当前log行数
    lineNums = 0;

    //允许最大行数
    m_maxLineNums = maxLineNums;

    //因为日志按天分，所以记录当前天数
    m_today = static_cast<uint64_t>(cur_tm.tm_mday);

    //是否开启异步
    m_asyn = asyn;

    //阻塞队列最大长度
    queue_MAXSIZE = queuesize;

    //获取阻塞队列
    std::shared_ptr<IFactory<std::string>> factory = std::make_shared<BlockQueueFactory<std::string>>();
    m_queue = factory->create();
    m_queue->setMaxSize(queue_MAXSIZE);

    //设置log文件为打开状态
    isOpen = true;

    //如果是异步写，创建一个写线程
    if(m_asyn){
        int err = pthread_create(&m_thread, NULL, pthread_func, NULL);
        if(err != 0){
            std::cerr << "create pthread error : " << err << std::endl;
            exit(1);
        }
        //设置线程为分离线程
        pthread_detach(m_thread);
    }
    return true;
}

//异步写线程函数，因为异步写线程只有一个，所以不用加锁。
void Log::asyn_write(){
    std::string line;
    while(isOpen){
        m_queue->pop(line);
        fstrm << line << std::endl;
        ++lineNums;
        if(lineNums % m_maxLineNums == 0 || m_today != m_timer.day){
            //创建新的文件
            newFile();
        }
    }
    fstrm.flush();
    fstrm.close();
}

//关闭log
void Log::close(){
    //写入log文件，表示用户主动关闭log
    m_queue->push("User Close Log\n");
    isOpen = false;
    //关闭文件，如果是异步写，写线程负责关闭文件，同步写当前线程负责关闭文件。
    if(!m_asyn){
        fstrm.flush();
        fstrm.close();
    }

}

void Log::wirte_log(LOG stat, const std::string message, bool *sucess){
    std::string line;
    switch (stat) {
        case LOG::DEBUG : {
            line = "[debug]";
            break;
        }
        case LOG::INFO : {
            line = "[info]";
            break;
        }
        case LOG::WARN : {
            line = "[warn]";
            break;
        }
        case LOG::ERROR : {
            line = "[error]";
            break;
        } 
        default : {
            line = "[info]";
            break;
        }
    }
    line += m_timer.getLocalTime();
    line += ":";
    line += message;

    *sucess = true;
    //同步写
    if(!m_asyn){
       syn_write(line);
    } else {
        *sucess = m_queue->push(line);
    }
    
}
void Log::syn_write(const std::string &line){
    MutexLockGuard lock(m_mutex);
    fstrm << line << std::endl;
    ++lineNums;
    if(lineNums % m_maxLineNums == 0 || m_today != m_timer.day){
        //创建新的文件
        newFile();
    }
}

void Log::newFile(){
    fstrm.flush();
    fstrm.close();
    if(!fstrm){
        std::cout << "close file error \n";
    }
    ++nfile;
    m_fileName += "_" + std::to_string(nfile);
    fstrm.open(m_fileName, std::ios_base::app);
    if(!fstrm){
        std::cout << "log file " << m_fileName << " open error " << fstrm.rdstate() << std::endl;
        exit(1);
    }
    lineNums = 0;
    m_today = m_timer.day;
}

#define LOG_DEBUG(m) Log::getInstance()->wirte_log(LOG::DEBUG, m, NULL);
#define LOG_DEBUG_B(m,n) Log::getInstance()->wirte_log(LOG::DEBUG, m, n);

#define LOG_INFO(m) Log::getInstance()->wirte_log(LOG::INFO, m, NULL);
#define LOG_INFO_B(m,n) Log::getInstance()->wirte_log(LOG::INFO, m, n);

#define LOG_WARN(m) Log::getInstance()->wirte_log(LOG::WARN, m, NULL);
#define LOG_WARN_B(m,n) Log::getInstance()->wirte_log(LOG::WARN, m, n);

#define LOG_ERROR(m) Log::getInstance()->wirte_log(LOG::ERROR, m, NULL);
#define LOG_ERROR_B(m,n) Log::getInstance()->wirte_log(LOG::ERROR, m, n);
