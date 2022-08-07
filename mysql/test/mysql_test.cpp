#include "../connect_pool.h"
#include "iostream"
using namespace std;
int main(){
    connection_pool* conn_p = connection_pool::GetInstance();
    conn_p->init("localhost", 3306, "zsz", "123456zx","zsz_data", 1);
    {
    ConnectRAII conn_raii(conn_p);

    cout << conn_p->GetFreeConn() << endl;
    mysql_query(conn_raii.getConn(),"set names utf8");
    
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