#ifndef IBLOCKQUEUE_H
#define IBLOCKQUEUE_H
#include <time.h>


template<typename T>
class IBlockQueue{
public:
    IBlockQueue(){}
    virtual ~IBlockQueue(){}
    virtual bool push(const T) = 0;
    virtual bool pop(T&) = 0;
    virtual bool pop(T&, struct timespec) = 0;

    virtual int setMaxSize(int) = 0;
    virtual int size() = 0;
    virtual int max_size() = 0;
};

#endif