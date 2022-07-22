#include "string"
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <queue>

using namespace std;
int main(){
    queue<int> a;
    a.push(10);
    cout << a.front() << ", " << a.size() << endl;
    a.pop();
    cout << a.size() << endl;

    return 0;
}
