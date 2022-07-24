#include "pthread.h"
#include "iostream"
#include <vector>
#include "unistd.h"

using namespace std;

class pool{
public:
    pool(int num);
    ~pool(){
        isOpen = false;
        for(int i = 0; i != m_npthread; ++i){
            pthread_join(m_pthread_list[i], nullptr);
        }
        pthread_mutex_destroy(&mutex);
    }
private:
    static void *work(void *);
    void run();
private:
    int m_npthread;
    bool isOpen;
    vector<pthread_t> m_pthread_list;
    pthread_mutex_t mutex;
    
};
pool::pool(int num):m_npthread(num), isOpen(true){
    m_pthread_list = vector<pthread_t>(num);
    pthread_mutex_init(&mutex, nullptr);
    for(int i = 0; i != m_npthread; ++i){
        pthread_create(&m_pthread_list[i], nullptr, work, this);
    }
    
}
void *pool::work(void *arg){
    pool* t = static_cast<pool *>(arg);
    t->run();
    return nullptr;
}
void pool::run(){
    int i = 0;
    while(isOpen){
        pthread_mutex_lock(&mutex);
        cout << i++ << endl;
        pthread_mutex_unlock(&mutex);
    }
    return;
}

int main(int argc, char* argv[]){
    pool p(3);
    sleep(1);
    return 0;
}