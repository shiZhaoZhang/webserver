//测试vector在构造函数中初始化的行为
#include <vector>
#include <iostream>
using namespace std;

class A{
public:
    A(int n);
    vector<int> m_list; 
};
A::A(int n): m_list{n}{}
int main(){
    A a(10);
    for(int i : a.m_list){
        cout << i << endl;
    }
    return 0;
}

/*
结论：
类构造函数:xxxx
后边就是一个初始化的过程
A::A(int n): m_list{n}
A::A(int n): m_list(n, 10)
*/