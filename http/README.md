# http结构
+ init():初始化,传入epollfd和需要监听的sockfd，边沿触发ET模式，注册EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT，并注意处理完之后要重新注册EPOLLONESHOT，并把sockfd设置为非阻塞模式
+ read_once()：完整读取传入的信息。完整读入信息之后放入。成功返回true，失败返回false
+ process(): 工作线程的任务：
    + http请求解析：
        + 分析，检查信息完整性，信息有错返回有错，信息不完整返回不完整
        + 信息完整尝试分析信息内容，使用状态机解析。
        + 所以http解析的结果有：INCOMPLETE, GET_REQUEST, BAD_REQUEST.
        + 如果是GET_REQUEST，之后对解析的结果进行判定，查看是否有权限，以及资源是否存在，故添加http解析结果类型：NO_RESOURCE,
        FORBIDDEN_REQUEST,
        INTERNAL_ERROR,
    + 对http的请求进行计算，并生成响应报文
    + 向epollfd注册写事件。
+ http发送请求:
    + 接受到写事件，向对应的sockfd发送
    + 判断connection的类型，对于HTTP/1.0默认关闭，1.1默认开启，之后决定是否关闭链接。