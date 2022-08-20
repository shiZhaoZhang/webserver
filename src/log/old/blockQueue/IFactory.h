#ifndef IFACTORY_H
#define IFACTORY_H
#include "memory"
#include "IBlockQueue.h"
template<typename T>
class IFactory{
public:
    virtual std::shared_ptr<IBlockQueue<T>> create() = 0;
};

#endif