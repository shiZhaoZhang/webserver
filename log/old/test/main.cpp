#include "string"
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include "../log.h"
#include <pthread.h>
#include <ctime>
using namespace std;
void *th0(void *val){
    for(int i = 0; i != *(int*)val; ++i){
        //cout << i << endl;
        LOG_DEBUG("test" + to_string(i) + "\n");
    }
    delete (int *)val;
    pthread_exit(nullptr);
}
int main(int argc, char *argv[]){
    bool isAsyn = true;
    int nSplice = 10000;
    int ncycle = 1000;
    if(argc < 4){
        std::cerr << "please input [asyn] [nSplice]" << endl;
        exit(1);
    }
    if(argv[1] != "asyn") {
        isAsyn = false;
    }
    nSplice = atoi(argv[2]);
    ncycle = atoi(argv[3]);
    clock_t start, end;
    start = clock();
    pthread_t thread[4];
    {
        Log* instance = Log::getInstance();
        instance->init(nSplice, isAsyn);
        for(int i = 0; i != 4; ++i)
        {
            int *val = new int(ncycle);
            pthread_create(thread + i, NULL, th0, (void*)(val));
            
        }
    }
    for(int i = 0; i != 4; ++i)
        pthread_join(thread[i], nullptr);
    Log::getInstance()->close();
    end = clock();
    cout << "The run time is: " << static_cast<double>(end - start) / CLOCKS_PER_SEC << endl;
    return 0;
}
