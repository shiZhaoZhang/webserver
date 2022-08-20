#ifndef CONNECTPOOL_H
#define CONNECTPOOL_H
#include "mysql/mysql.h"
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <list>
#include <functional>
#include <string>
#include "rlog.h"
#include "openssl/sha.h"

class ConnectRAII;

class connection_pool{
    friend class ConnectRAII;
public:
    //获取数据库池对象智能指针。
    static connection_pool* GetInstance();
    //初始化
    void init(std::string host, int port, std::string user_name, std::string password, std::string db_name, int max_conn);
    
    //获取当前可用的连接数目
    int GetFreeConn(){
        return m_free_conn;
    }
private:
    connection_pool():m_free_conn(0), isInit(false){}
    ~connection_pool();
    //通过设置为私有函数，强制使用ConnectRAII来获取数据库
    //获取一个数据库连接
    MYSQL *GetConnection();
    //释放一个连接
    bool ReleaseConnection(MYSQL* &);
private:
    //可用数据库连接列表
    std::list<MYSQL*> m_list;
    //可用连接数目
    int m_free_conn;
    //条件变量，用于连接的获取和释放
    std::condition_variable m_con;
    //互斥锁，用于获取连接和释放连接时候的
    std::mutex m_mutex;
    //
    bool isInit;
};

//RAII封装connectPool
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
/****************************************************************\
 *                           数据库操作函数                         *
\****************************************************************/
//查看用户名是否存在,成功返回密码，失败返回空
std::string SearchUserAndPasswd(const char*user_name, MYSQL *mysql);

//添加
bool InsertUser(const char* user_name, const char *passwd, MYSQL *mysql);

//sha512加密
std::string SHA512(const char* meaasge);
#endif