#include "connect_pool.h"

//c++11之后的线程安全懒汉模式
connection_pool* connection_pool::GetInstance(){
    static connection_pool *m_instance = new connection_pool();
    return m_instance;
}

//析构函数
connection_pool::~connection_pool(){
    std::lock_guard<std::mutex> lock(m_mutex);
    for(auto iter = m_list.begin(); iter != m_list.end(); ++iter){
        //关闭mysql连接
        if(*iter != nullptr)
            mysql_close(*iter);
    }
}

//初始化，由于设置isInit变量，即使多次调用init函数，也只有第一次调用的时候有效果。
void connection_pool::init(std::string host, int port, std::string user_name, std::string password, std::string db_name, int max_conn){
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        //如果已经初始化，返回
        if(isInit){
            return;
        }
        isInit = true;
        //设置最大连接数，防止错误的输入
        if(max_conn <= 0)
            max_conn = 4;
        //获取数据库连接
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
        
    }
    //初始化数据库，创建user表，考虑当前只需要保存用户信息，所以只用构建user表，采用内置在代码中。
    {
        MYSQL *m_mysql = GetConnection();
        if(!m_mysql){
            printf("mysql init error\n");
            exit(0);
        }
        std::string s = 
        "CREATE TABLE IF NOT EXISTS `user`( "
        "`user_id` INT UNSIGNED AUTO_INCREMENT,"
        "`user_name` VARCHAR(100) NOT NULL,"
        "`passwd` VARCHAR(64) NOT NULL,"
        "`submission_date` DATE,"
        "PRIMARY KEY ( `user_id` )"
        ")ENGINE=InnoDB DEFAULT CHARSET=utf8;";
        mysql_query(m_mysql, s.c_str());
        ReleaseConnection(m_mysql);
    }

}

//获取连接
MYSQL *connection_pool::GetConnection(){
    MYSQL *res = nullptr;
    std::unique_lock<std::mutex> lck(m_mutex);
    m_con.wait(lck, [this]()->bool{return this->m_free_conn > 0;});
    res = m_list.front();
    m_list.pop_front();
    --m_free_conn;
    
    return res;
}


//释放连接
bool connection_pool::ReleaseConnection(MYSQL *&conn){
    if(conn == nullptr) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_list.push_back(conn);
    ++m_free_conn;
    m_con.notify_all();
    conn = nullptr;
    return true;
}


