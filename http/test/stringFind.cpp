//测试string的find类型查找函数
#include "string"
#include "iostream"
#include "algorithm"

using namespace std;

void testFindOf();
void testUpper();

int main(){
    cout << (int)'a' << ", " << (int)'A' << endl;
    testUpper();
    
}
void testFindOf(){
    string request = "GET / HTTP/1.1";
    string f = " \t";
    int start = 0;
    while (request.find_first_of(f, start) != string::npos)
    {
        start = request.find_first_of(f, start);
        cout << start++ << endl;
    }
}

void testUpper(){
    string temp_method = "sad21|HSUIHIS123412@%^#";
    transform(temp_method.begin(),temp_method.end(),temp_method.begin(),::toupper);
    cout << temp_method << endl;
}