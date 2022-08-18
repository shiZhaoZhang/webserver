#include "../threadPool.h"
#include "../../http/httpconn.h"
#include "../../mysql/connect_pool.h"
int main(){
    connection_pool::GetInstance()->init("localhost", 3306, "zsz", "123456zx","zsz_data", 10);
    //创建线程池
    std::shared_ptr<threadPool<http>> m_threadPool = std::make_shared<threadPool<http>>(connection_pool::GetInstance(), 4, 20);
    return 0;
}