//测试如果throw std::exception没有接受会不会直接退出
#include <vector>
#include <iostream>
using namespace std;
#define TEST "test"
#define STRING "string"

#define CON TEST STRING "connect"

class A{
public:
    A(int n);
    vector<int> m_list; 
};
A::A(int n): m_list{n}{
    if(n == 0){
        throw exception();
    }
}
int main(){
    A a(1);
    for(int i : a.m_list){
        cout << i << endl;
    }
    cout << CON << endl;
    std::cerr <<  "[" << __FILE__ << "]: " << __LINE__ << " "<<  __FUNCTION__ << std::endl;
    return 0;
}
/*
直接报告错误并退出
terminate called after throwing an instance of 'std::exception'
  what():  std::exception
已放弃 (核心已转储)

define的字符串连接，中间加空格
#define TEST "test"
#define STRING "string"

#define CON TEST STRING "connect"

CON teststringconnect
*/
