#include "../threadPool.h"
#include "../../http/httpconn.h"
int main(){
    connection_pool::GetInstance()->init("localhost", 3306, "zsz", "123456zx","zsz_data", 10);
    //创建线程池
    threadPool<http> m_threadPool(connection_pool::GetInstance(), 4, 20);
    return 0;
}