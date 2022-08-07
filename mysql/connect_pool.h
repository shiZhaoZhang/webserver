#ifndef CONNECTPOOL_H
#define CONNECTPOOL_H
#include "mysql/mysql.h"
#include "../lock/locker.h"
#include "list"
#include "string"

class connection_pool{
public:
    //获取数据库池对象指针。
    static connection_pool *GetInstance();
    //初始化
    void init(std::string host, int port, std::string user_name, std::string password, std::string db_name, int max_conn);
    //获取一个数据库连接
    MYSQL *GetConnection();
    //释放一个连接
    bool ReleaseConnection(MYSQL* &);
    //获取当前可用的连接数目
    int GetFreeConn(){
        return m_free_conn;
    }
private:
    connection_pool();
    ~connection_pool();
private:
    //可用数据库连接列表
    std::list<MYSQL*> m_list;
    //可用连接数目
    int m_free_conn;
    //信号量，用于连接的获取和释放
    sem m_sem;
    //互斥锁，用于获取连接和释放连接时候的
    locker m_locker;
};
/*实现*/
connection_pool::connection_pool():m_free_conn(0){}
connection_pool::~connection_pool(){
    MutexLockGuard lock(m_locker);
    for(auto iter = m_list.begin(); iter != m_list.end(); ++iter){
        //关闭连接
        if(*iter != nullptr)
            mysql_close(*iter);
    }
}

//c++11之后的线程安全懒汉模式
connection_pool *connection_pool::GetInstance(){
    static connection_pool m_instance;
    return &m_instance;
}

//多次init会导致不可预测的后果
void connection_pool::init(std::string host, int port, std::string user_name, std::string password, std::string db_name, int max_conn){
    MutexLockGuard lock(m_locker);
    
    if(max_conn <= 0)
        max_conn = 3;
    
    for(int i = 0; i != max_conn; ++i){
        MYSQL *mysql_handler = nullptr;
        mysql_handler = mysql_init(mysql_handler);
        if(!mysql_handler){
            printf("mysql connect error\n");
            continue;
        }
        mysql_handler = mysql_real_connect(mysql_handler, host.c_str(), user_name.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0);
        if(!mysql_handler){
            printf("mysql connect error\n");
            continue;
        }
        m_list.push_back(mysql_handler);
        ++m_free_conn;
              
    }
    //再次给信号量赋值
    m_sem = sem(m_free_conn);
    
}

//获取连接
MYSQL *connection_pool::GetConnection(){
    MYSQL *res = nullptr;
    if(m_list.empty())
        return res;
    m_sem.wait();
    {
        MutexLockGuard lock(m_locker);
        res = m_list.front();
        m_list.pop_front();
        --m_free_conn;
    }
    return res;
}

//释放连接
bool connection_pool::ReleaseConnection(MYSQL *&conn){
    if(conn == nullptr) return false;
    MutexLockGuard lock(m_locker);
    m_list.push_back(conn);
    ++m_free_conn;
    m_sem.post();
    conn = nullptr;
    return true;
}


class ConnectRAII{
public:
    ConnectRAII(connection_pool* connectpool){
        m_mysql = connectpool->GetConnection();
        m_conn_pool = connectpool;
    }
    ~ConnectRAII(){
        m_conn_pool->ReleaseConnection(m_mysql);
    }
    MYSQL* getConn(){
        return m_mysql;
    }
private:
    MYSQL* m_mysql;
    connection_pool* m_conn_pool;
};
#endif