#include "iostream"
#include "fstream"
#include "string"
#include <sys/stat.h>

using namespace std;

//辅助函数：判断文件是否存在
/*bool isFileExists_stat(const std::string& name) {
  struct stat buffer;   
  return (stat(name.c_str(), &buffer) == 0); 
}*/

int main(){
    //判断当前log文件夹是否存在，不存在则创建
    /*if(!isFileExists_stat("log")){
        mkdir("log", 0755);
    }
    fstream fstrm;
    string m_fileName = "log/123";
    fstrm.open(m_fileName, std::ios_base::app);
    if(!fstrm){
        cout << "open error" << endl;
    }
    cout << "hello " << endl;*/


    return 0;
}