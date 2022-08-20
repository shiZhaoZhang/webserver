# http结构
+ init():初始化,传入epollfd和需要监听的sockfd，边沿触发ET模式，注册EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT，并注意处理完之后要重新注册EPOLLONESHOT，并把sockfd设置为非阻塞模式
+ read_once()：完整读取传入的信息。完整读入信息之后放入。成功返回true，失败返回false
+ process(): 工作线程的任务：
    + http请求解析：
        + 分析，检查信息完整性，信息有错返回有错，信息不完整返回不完整
        + 信息完整尝试分析信息内容，使用状态机解析。
        + 所以http解析的结果有：OPEN_REQUEST, GET_REQUEST, BAD_REQUEST.
        + 如果是GET_REQUEST，之后对解析的结果进行判定，查看是否有权限，以及资源是否存在，故添加http解析结果类型：NO_RESOURCE,
        FORBIDDEN_REQUEST,
        INTERNAL_ERROR,
    + 对http的请求进行计算，并生成响应报文
    + 向epollfd注册写事件。
+ http发送请求:
    + 接受到写事件，向对应的sockfd发送
    + 判断connection的类型，对于HTTP/1.0默认关闭，1.1默认开启，之后决定是否关闭链接。

# 流程
## 主线程
+ 持有一个map，sockfd和http一一对应。
+ 监听到可读，在map查找是否有对应的sockfd，没有就构建http智能指针，并添加到map。使用read_once读取，读完之后把http添加到队列中
## 工作线程：
+ 使用process函数，函数先分析请求，如果http不完整，直接跳过，等到下次读取从尾部继续添加。如果请求有错误，发送`400 Bad Request`相应。否则进入第三步：URL。
+ URL: 查看所提供的URL是否支持，支持转移到相应的函数，不支持返回`404 Not Found`。
+ 附加：最后查看`connction`子段，如果是close，执行完毕之后关闭连接。
+ 构造响应报文之后并不在工作线程发送报文，而是注册写事件，主线程检测到写事件，调用sockfd对应的http对象的write函数，如果connction=close，发送完毕关闭连接，否则重新注册读事件。

# URL对应函数
+ '/' : GET请求，返回响应 judge.html 页面 (默认界面)
+ '/0': POST请求，返回响应 register.html 页面 (注册)
+ '/1': POST请求，返回响应 log.html 页面 (登录)
+ '/2CGISQL.cgi' ：POST请求，携带参数`user`和`password`，进行登录校验，成功返回`welcome.html`，失败返回`logErr.html`。
+ '/3CGISQL.cgi'：POST请求，进行注册校验，携带参数`user`和`password`，进行注册校验，是否同名，成功返回`log.html`，失败返回`registerError.html`。
+ '/5': POST请求，跳转到`picture.html`，即图片请求页面。
+ '/6': POST请求，跳转到`video.html`，即视频请求页面
+ '/7': POST请求，跳转到`fans.html`，即关注页面

# 8/21 更新：
# 添加静态资源托管
通过http::staticSource可以托管静态资源，staticSource是一个map<string, string>，其中first是url，second是对应资源的位置。
例如：
http::staticSource.insert({"/5", "./root/picture.html"});
注意，静态资源都只能用GET获取，POST清使用http::registerPost注册对应的函数

# 添加GET请求对应url的处理函数
使用http::registerGet来添加GET请求对应URL的处理，其类型是std::map<std::string, void (*) (http_request&, http_response&, MYSQL*)> ，first是对应的url，second是注册的函数指针，其中函数的第一个参数是http报文解析结果，第二个参数用于构建http报文，第三个参数是数据库指针，这个函数会在process的时候调用。
例如：
`http::registerGet.insert({"/2CGISQL.cgi", *userLoginFunc});`
其中
```cpp
void userLoginFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql){
    auto params = m_request.get_params();
    if(params.find("user") == params.end() || params.find("password") == params.end())
    {
        m_response.base_request("400 Bad Request");
        m_response.add_Server("Bowu.server v1.0");
        m_response.add_ContentLength(0);
        m_response.end_response_message_head();
    } else {
        auto user_name = params["user"];
        auto passwd = params["password"];
        std::string res = SearchUserAndPasswd(user_name.c_str(), m_mysql);
        //加密之后对比
        passwd = SHA512(passwd.c_str());
        m_response.base_request("200 OK");
        m_response.add_Server("Bowu.server v1.0");
        if((res == "") || (res != passwd)){
            m_response.add_file("./root/logError.html");
            m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
        } else {
            m_response.add_file("./root/welcome.html");
            m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
        }
        m_response.end_response_message_head();
    }
    return;
}
```cpp

# POST同理，不过使用的是`std::map<std::string, void (*) (http_request&, http_response&, MYSQL*)> http::registerPost;`
