//函数类型转换运算符
#include <iostream>
using namespace std;
class A{
public:
   operator int(){
    return b + 10;
  }  
  int b;
};
int main(){
    A a;
    a.b = 10;
    int b = a;
    cout << b << endl;
}
/*
operator int()实际上定义了A到int的转换。
所以可以直接把A类型的值赋值给int类型。
*/