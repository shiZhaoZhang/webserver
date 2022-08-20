#ifndef CIP_H
#define CIP_H
#include "map"
#include "string"
#include "mysql/mysql.h"
#include "string.h"

//保存真实的ip地址的位置，在每一个tick()的时候，查看保存的ip地址是否在数据库中，在的话访问次数+1，不在就创建并保存在数据库中。
class CIP{
public:
    CIP(MYSQL* mysql) : m_mysql(mysql){

    }
    ~CIP(){

    }
    //添加ip
    void add(std::string ip){
        ++real_ip[ip];
    }
    //持久化，写入数据库
    void write();
private:
    std::map<std::string, int> real_ip;
    MYSQL* m_mysql;
};
void CIP::write(){
    char m[100];
    for(auto i = real_ip.begin(); i != real_ip.end(); ++i){

        memset(m, '\0', 100);
        snprintf(m, 99, "select * from read_ip where ip = \"%s\"", i->first.c_str());
        if(mysql_query(this->m_mysql, m))        //执行SQL语句
        {
            return;
        }
    
        MYSQL_RES *res;
        res = mysql_store_result(m_mysql);
        //没找到，添加
        if(mysql_affected_rows(this->m_mysql) != 1){
            memset(m, '\0', 100);
            snprintf(m, 99, "INSERT INTO read_ip (ip, nums, last_data) values (\"%s\", %d, NOW());", i->first.c_str(), i->second);
            if(mysql_query(this->m_mysql, m))        //执行SQL语句
            {
                
                mysql_free_result(res);
                return;
            }
            
        }
        //找到，增加
        else{
            //更新访问次数
            memset(m, '\0', 100);
            snprintf(m, 99, "update read_ip set nums=nums+%d where ip=\"%s\";", i->second, i->first.c_str());
            if(mysql_query(this->m_mysql, m))        //执行SQL语句
            {
                mysql_free_result(res);
                return;
            }
            //更新访问时间
            memset(m, '\0', 100);
            snprintf(m, 99, "update read_ip set last_data=NOW() where ip=\"%s\"", i->first.c_str());
            if(mysql_query(this->m_mysql, m))        //执行SQL语句
            {
                
                mysql_free_result(res);
                return;
            }
        }
    
    }

    real_ip.clear();

}
#endif