#include "../connect_pool.h"
#include "iostream"
#include "string.h"

using namespace std;
string ExistUser(MYSQL *m_sql, const char*user_name){
    char m[100];
    memset(m, '\0', 100);
    snprintf(m, 99, "select * from user where user_name = \"%s\"",
                    user_name);
    if(mysql_query(m_sql, m))        //执行SQL语句
    {
        printf("Query failed (%s)\n",mysql_error(m_sql));
        return "";
    }
    else
    {
        printf("query success\n");
    }
    MYSQL_RES *res;
    res = mysql_store_result(m_sql);
    cout << "row num = " << mysql_affected_rows(m_sql) << endl;
    if(mysql_affected_rows(m_sql) == 1){
        
        //res = mysql_store_result(m_sql);
        MYSQL_ROW m_row = mysql_fetch_row(res);
        printf("passwd = %s\n", m_row[2]);
        string ps = m_row[2];
        mysql_free_result(res);
        return ps;
    }
    mysql_free_result(res);
    return "";

}
bool InsertUser(MYSQL *m_sql, const char*user_name, const char* passwd)
{
    char m[100];
    memset(m, '\0', 100);
    snprintf(m, 99, "INSERT INTO user (user_name, passwd, submission_date)"
                    "VALUES"
                    "(\"%s\", \"%s\", NOW());",
                    user_name, passwd);
    //添加
    if(mysql_query(m_sql, m))        //执行SQL语句
    {
        printf("Query failed (%s)\n",mysql_error(m_sql));
        return false;
    }
    else
    {
        printf("query success\n");
    }
    return true;
}
int main(){
    connection_pool* conn_p = connection_pool::GetInstance();
    conn_p->init("localhost", 3306, "zsz", "123456zx","zsz_data", 1);
    {
    ConnectRAII conn_raii(conn_p);
    mysql_query(conn_raii.getConn(),"set names utf8");

    string ps = ExistUser(conn_raii.getConn(), "user0");
    if(ps == ""){
        printf("user0 is exist, passwd = %s\n", ps.c_str());
    }

    
   
    if(mysql_query(conn_raii.getConn(), "select * from runoob_tbl where runoob_id<10"))        //执行SQL语句
    {
        printf("Query failed (%s)\n",mysql_error(conn_raii.getConn()));
        return false;
    }
    else
    {
        printf("query success\n");
    }
    MYSQL_RES *res;
    res = mysql_store_result(conn_raii.getConn());
    //打印数据行数
    printf("number of dataline returned: %d\n",mysql_affected_rows(conn_raii.getConn()));
    printf("number of column returned: %d\n",mysql_field_count(conn_raii.getConn()));
    }
    
    cout << conn_p->GetFreeConn() << endl;
    
    return 0;
}