#include "time.h"
#include "stdio.h"
#include "string"
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
    printf("string len = %d, %s\n", fM.size(), fM.c_str());
    return 0;
}