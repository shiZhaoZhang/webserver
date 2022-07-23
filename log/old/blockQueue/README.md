## 用生产者——消费者模型构建的阻塞队列
### 亮点：
+ 使用生产者——消费者模型构建阻塞队列

+ 使用`MAX_SIZE`限制了队列的长度

+ push用于从队列尾部添加
    + 队列如果满了，就发起广播，通知可能处于等待状态的pop，并返回一个false表示添加失败
    + 正常情况，添加到尾部并发起一个通知。
+ pop系列函数用于从队列头部拉取
    + 如果队列为空，则等待通知
    + 正常情况直接从头部取出一个元素返回。

+ 数据结构使用std的queue，简化push和pop操作

+ 对于区域内的加解锁使用`MutexLockGuard`简化代码，防止遗忘解锁


# 工厂模式
考虑到block_queue的实现可能有多种，所以使用了工厂模式
+ IFactory.h：抽象工厂类，提供create接口
+ blockQueueFactory.h：具体工厂类，实现create接口，返回一个IblockQueue抽象对象类的指针。
+ IblockQueue.h：抽象阻塞队列
    + push(T): 压入数据
    + pop(T&)：取出数据
    + pop(T&, struct timespec)：等待的取出数据
    + size() ：当前数量
    + max_size():允许最大数量
    + setMaxSize()：设置允许的最大数量
+ blockQueue.h：具体实现的阻塞队列