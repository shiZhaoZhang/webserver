#include "httpconn.h"


std::map<std::string, void (*) (http_request&, http_response&, MYSQL*)> http::registerGet;
std::map<std::string, void (*) (http_request&, http_response&, MYSQL*)> http::registerPost;
std::map<std::string, std::string> http::staticSource = {{"/404.JPG", "./root/404.JPG"}, {"/", "./root/judge.html"},{"/favicon.ico", "./root/favicon.ico"},
                                                            {"/xxx.jpg", "./root/xxx.jpg"}, {"/TurnAroundFish.mp4", "./root/TurnAroundFish.mp4"}};

//初始化
bool http::init(int epollfd, int sockfd, SSL* ssl){
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    m_ssl = ssl;
    //注册失败
    if(epollAdd(m_epollfd, m_sockfd, EPOLLIN) == -1){
        return false;
    }
    return true;
}

//清空
void http::clearData(){
    m_message.clear();
    request_parase.clear();
    respond_message.clear();
    respond_body.clear();
    if(m_iovec[1].iov_base){
        munmap(m_iovec[1].iov_base, m_iovec[1].iov_len);
    }
}

//读取sockfd中的信息
READ_STAT http::read_once(){
    std::string buffer(BUFFER_SIZE, '\0');
    //由于采用的是ET模式，所以需要一次性读完
    ssize_t len = 0;
    while(true){
        //判断是不是https连接
        ssize_t ret;
        if(m_ssl){
            ret = SSL_read(m_ssl, &*buffer.begin() + len, BUFFER_SIZE - len);
        } else {
            ret = recv(m_sockfd, &*buffer.begin() + len, BUFFER_SIZE - len, 0);
        }
        if(ret < 0){
            //对于非阻塞IO，下边条件成立代表数据全部读完，跳出循环
            if((errno == EAGAIN)||(errno == EWOULDBLOCK)){
                break;
            }
            //否则读取失败
            return READ_STAT::BAD;
        } else if(ret == 0){
            //判断是不是buffer的剩余空间不够了，是的话先把结果存在message，把buffer清空，继续读
            if(len == BUFFER_SIZE){
                m_message += buffer;
                buffer = std::string(BUFFER_SIZE, '\0');
                len = 0;
                continue;
            }
            return READ_STAT::CLOSE;
        } else {
            len += ret;
        }
    }
    m_message += buffer.substr(0, len);
    return READ_STAT::OK;
}
//单独传送第n个缓冲区,2代表传输成功 1代表出现阻塞，0代表失败，
int http::https_write_one_buf(int n){
    DEBUG("ssl_write %d", n);
    size_t len = m_iovec[n].iov_len;
    if(len > 0){
        while(true){
            ssize_t temp_len = SSL_write(m_ssl, m_iovec[n].iov_base, m_iovec[n].iov_len);
            if(temp_len < 0){
                //出现阻塞，重新触发写事件，下次写
                if(errno == EAGAIN){
                    modfd(m_epollfd, m_sockfd, EPOLLOUT);
                    DEBUG("EAGAIN");
                    return 1;
                } 
                //出现问题
                DEBUG("some worng");
                clearData();
                return 0;
            }
            DEBUG("len = %d, send_len = %d", (int)len, (int)temp_len);
            len -= (size_t)temp_len;
            if(len <= 0){
                m_iovec[n].iov_base = (void*)((char*)m_iovec[n].iov_base + m_iovec[n].iov_len);
                m_iovec[n].iov_len = 0;
                break;
            }
            //没有传完
            m_iovec[n].iov_base = (void*)((char*)m_iovec[n].iov_base + temp_len);
            m_iovec[n].iov_len -= temp_len;
        }
    }
    DEBUG("ssl_write %d success", n);
    return 2;
}
bool http::https_write_once(){
    INFO("https write:%s", (char*)m_iovec[0].iov_base);
    for(int i = 0; i < 2; ++i){
        int ret = https_write_one_buf(i);
        if(ret == 0) return false;
        else if(ret == 1) return true;
    }
    //传输完成
    std::string find_s = "Connection";
    auto header_ = request_parase.get_head_params();
    auto iter = header_.find(find_s);
    
    //长连接
    if(iter == request_parase.get_head_params().end() || iter->second == "keep-alive"){
        
        clearData();
        //继续监听m_sockfd套接字，重新触发读事件，移除写事件
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        DEBUG("keep-alive");
        return true;
    }
    //短连接
    clearData();
    DEBUG("shot connect");
    return false;
}

//主线程：往socket写
bool http::http_write_once(){
    INFO("http write:%s", (char*)m_iovec[0].iov_base);
    size_t len = m_iovec[0].iov_len + m_iovec[1].iov_len;
    while(true){
        ssize_t temp_len = writev(m_sockfd, m_iovec, 2);
        if(temp_len < 0){
            //出现阻塞，重新触发写事件，下次写
            if(errno == EAGAIN){
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            } 
            //出现问题
            clearData();
            return false;
        }
        len -= (size_t)temp_len;
        //数据写完
        if(len == 0){
            std::string find_s = "Connection";
            auto header_ = request_parase.get_head_params();
            auto iter = header_.find(find_s);
            
            //长连接
            if(iter == request_parase.get_head_params().end() || iter->second == "keep-alive"){
                
                clearData();
                //继续监听m_sockfd套接字，重新触发读事件，移除写事件
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                
                return true;
            }
            //短连接
            clearData();
            return false;
        }

        //没有传完，修改传输的起始位置
        if(temp_len <= m_iovec[0].iov_len){
            m_iovec[0].iov_base = (void*)((char*)m_iovec[0].iov_base + temp_len);
            m_iovec[0].iov_len -= temp_len;
        } else {
            m_iovec[1].iov_base = (void*)((char*)m_iovec[1].iov_base + temp_len - m_iovec[0].iov_len);
            m_iovec[1].iov_len -= temp_len - m_iovec[0].iov_len;

            m_iovec[0].iov_base = (void*)((char*)m_iovec[0].iov_base + m_iovec[0].iov_len);
            m_iovec[0].iov_len = 0;
        }

    }
}


void http::process(){
    //解析报文
    HTTP_CODE code = this->request_parase.parser(this->m_message);
    DEBUG("[request]%s", this->m_message.c_str());
    //根据报文不同的解析结果返回不同的响应报文。
    switch (code)
    {
    case HTTP_CODE::OPEN_REQUEST:
        return;
        break;
    case HTTP_CODE::BAD_REQUEST:
        bad_request();
        break;
    case HTTP_CODE::INTERNAL_ERROR:
        internal_error();
        break;
    case HTTP_CODE::GET_REQUEST:
        get_request();
        break;
    default:
        break;
    }
    
    //触发epollfd写事件
    if(modfd(m_epollfd, m_sockfd, EPOLLOUT) == -1){
        return;
    }
}

void http::bad_request(){
    m_response.base_request("400 Bad Request");
    m_response.add_Server("Bowu.server v1.0");
    m_response.add_ContentLength(0);
    m_response.end_response_message_head();
    m_iovec[0].iov_base = (void*)(m_response.response_message_head.c_str());
    m_iovec[0].iov_len = m_response.response_message_head.size();
    m_iovec[1].iov_base = nullptr;
    m_iovec[1].iov_len = 0;
    return;
}
void http::internal_error(){
    m_response.base_request("500 Internal Server Error");
    m_response.add_Server("Bowu.server v1.0");
    m_response.add_ContentLength(0);
    m_response.end_response_message_head();
    m_iovec[0].iov_base = (void*)(m_response.response_message_head.c_str());
    m_iovec[0].iov_len = m_response.response_message_head.size();
    m_iovec[1].iov_base = nullptr;
    m_iovec[1].iov_len = 0;
    return;
}
void http::noFound(){
    m_response.base_request("404 Not Found");
    m_response.add_Server("Bowu.server v1.0");
    m_response.add_file("root/404NotFound.html");
    m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
    m_response.end_response_message_head();
    m_iovec[0].iov_base = (void*)(m_response.response_message_head.c_str());
    m_iovec[0].iov_len = m_response.response_message_head.size();
    m_iovec[1].iov_base = m_response.response_message_body_file->getMemory();
    m_iovec[1].iov_len = m_response.response_message_body_file->get_fileLength();
    return;
}

void http::get_request(){
    //printf("request:\n%s\n..............................\n", m_message.c_str());
    std::string m_url = request_parase.get_URL();
    
    if(request_parase.get_method() == "GET"){
        //静态资源
        if(staticSource.find(m_url) != staticSource.end()){
            LOG_INFO("Get source %s", staticSource[m_url].c_str());
            //找到资源
            m_response.base_request("200 OK");
            m_response.add_Server("Bowu.server v1.0");
            m_response.add_file(staticSource[m_url]);
            m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
            m_response.end_response_message_head();
            
            m_iovec[0].iov_base = (void*)(m_response.response_message_head.c_str());
            m_iovec[0].iov_len = m_response.response_message_head.size();
            m_iovec[1].iov_base = m_response.response_message_body_file->getMemory();
            m_iovec[1].iov_len = m_response.response_message_body_file->get_fileLength();

            return;
        } else if(registerGet.find(m_url) != registerGet.end()){
            LOG_INFO("Get %s", m_url.c_str());
            registerGet[m_url](this->request_parase, this->m_response, mysql);
            m_iovec[0].iov_base = (void*)(m_response.response_message_head.c_str());
            m_iovec[0].iov_len = m_response.response_message_head.size();
            m_iovec[1].iov_base = m_response.response_message_body_file->getMemory();
            m_iovec[1].iov_len = m_response.response_message_body_file->get_fileLength();
            return ;
        }
    } else if(request_parase.get_method() == "POST" && registerPost.find(m_url) != registerPost.end()){
        LOG_INFO("Post %s", m_url.c_str());
        registerPost[m_url](this->request_parase, this->m_response, mysql);
        m_iovec[0].iov_base = (void*)(m_response.response_message_head.c_str());
        m_iovec[0].iov_len = m_response.response_message_head.size();
        m_iovec[1].iov_base = m_response.response_message_body_file->getMemory();
        m_iovec[1].iov_len = m_response.response_message_body_file->get_fileLength();
        return ;
    } 
    LOG_INFO("No Found url[%s]", m_url.c_str());
    noFound();
}
/****************************************************************\
 *                           辅助函数                             *
\****************************************************************/



//成功返回旧值，失败返回-1。
int http::setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    int err = fcntl(fd, F_SETFL, new_option);
    return err == -1? -1 : old_option;
}

//成功返回0，失败返回-1
int http::epollAdd(int epollfd, int sockfd, int ev){
    struct epoll_event event;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    event.data.fd = sockfd;
    //把sockfd设置为非阻塞
    if(setnonblocking(sockfd) == -1){
        return -1;
    }
    //注册
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1){
        return -1;
    }
    return 0;
}


//成功返回0，失败返回-1
int http::modfd(int epollfd, int sockfd, int ev){
    struct epoll_event event;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    event.data.fd = sockfd;
    //修改
    if(epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event) == -1){
        return -1;
    }
    return 0;
}

/****************************************************************\
 *                           数据库操作函数                         *
\****************************************************************/
//添加
bool http::InsertUser(const char* user_name, const char *passwd){
    char m[100];
    memset(m, '\0', 100);
    snprintf(m, 99, "INSERT INTO user (user_name, passwd, submission_date)"
                    "VALUES"
                    "(\"%s\", \"%s\", NOW());",
                    user_name, passwd);
    //添加
    if(mysql_query(this->mysql, m))        //执行SQL语句
    {
        //printf("Query failed (%s)\n",mysql_error(conn_raii.getConn()));
        return false;
    }
    else
    {
        //printf("query success\n");
    }
    return true;
}   
//查看用户名是否存在,成功返回密码，失败返回空
std::string http::ExistUser(const char*user_name){
    char m[100];
    memset(m, '\0', 100);
    snprintf(m, 99, "select * from user where user_name = \"%s\"", user_name);
    if(mysql_query(this->mysql, m))        //执行SQL语句
    {
        //printf("Query failed (%s)\n",mysql_error(m_sql));
        return "";
    }
    else
    {
        //printf("query success\n");
    }
    MYSQL_RES *res;
    res = mysql_store_result(this->mysql);
    if(mysql_affected_rows(this->mysql) != 1){
        mysql_free_result(res);
        return "";
    }
    MYSQL_ROW m_row = mysql_fetch_row(res);
    std::string passwd = m_row[2];
    mysql_free_result(res);
    return passwd;
}

