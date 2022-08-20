#ifndef BLOCKQUEUEFACTORY_H
#define BLOCKQUEUEFACTORY_H
#include "IFactory.h"
#include "blockQueue.h"
template<typename T>
class BlockQueueFactory: public IFactory<T>{
public:
    std::shared_ptr<IBlockQueue<T>> create(){
        return std::make_shared<block_queue<T>>();
    }
};

#endif