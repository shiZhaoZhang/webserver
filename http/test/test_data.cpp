#include "time.h"
#include "stdio.h"
#include "string"
#include "iostream"
using namespace std;
int main(){
    time_t rawTime;
    struct tm* timeInfo;
    char szTemp[36]={'\0'};
    time(&rawTime);
    timeInfo = gmtime(&rawTime);
    strftime(szTemp,sizeof(szTemp),"%a, %d %b %Y %H:%M:%S GMT",timeInfo);

    string fM = "123";
    fM += szTemp;
    fM += "123";
    fM.insert(fM.size()-3, "----");
    cout << fM << endl;
    return 0;
}