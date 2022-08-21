#include "server.h"
void webserver::configParser(){
    std::ifstream file_in("config/webserver.config");
    if(!file_in){
        ERROR("need file[config] in folder[config]: ./config/webserver.config\nformate:[xxxx:xxx]\n"
            "mysql_ip: localhost"
            "mysql_port: 3306"
            "mysql_username: database_user_name"
            "passwd : database_passwd"
            "database: database_name"
            "mysql_max_connect_numbers: 20"
            "thread_pool_max_list: 1000"
            "thread_pool_nums: 4");
        return;
    }
    std::string line;
    std::map<std::string, std::string> data;
    while(getline(file_in, line)){
        size_t index = line.find(':',0);
        size_t start = line.find_first_not_of(' ');
        size_t end = line.find_last_not_of(' ', index - 1);
        std::string a_ = line.substr(start, end - start + 1);
        start = line.find_first_not_of(' ', index + 1);
        end = line.find_last_not_of(' ');
        std::string b_ = line.substr(start, end - start + 1);
        data[a_] = b_;
    }
    bool flag = false;
    if(data.find("mysql_username") != data.end()){
        mysql_username = data["mysql_username"];
    } else {
        flag = true;
    }
    if(data.find("passwd") != data.end()){
        mysql_passwd = data["passwd"];
    } else {
        flag = true;
    }
    if(data.find("database") != data.end()){
        mysql_database = data["database"];
    } else {
        flag = true;
    }
    if(data.find("mysql_max_connect_numbers") != data.end()){
        mysql_max_connect_numbers = atoi(data["mysql_max_connect_numbers"].c_str());
    } else {
        flag = true;
    }
    if(data.find("thread_pool_max_list") != data.end()){
        thread_pool_max_list = atoi(data["thread_pool_max_list"].c_str());
    } else {
        flag = true;
    }
    if(data.find("thread_pool_nums") != data.end()){
        thread_pool_nums = atoi(data["thread_pool_nums"].c_str());
    } else {
        flag = true;
    }
    if(data.find("mysql_ip") != data.end()){
        mysql_ip = data["mysql_ip"].c_str();
    } else {
        flag = true;
    }
    if(data.find("mysql_port") != data.end()){
        mysql_port = atoi(data["mysql_port"].c_str());
    } else {
        flag = true;
    }
    if(flag){
        ERROR("need file[config] in folder[config]: ./config/webserver.config\nformate:[xxxx:xxx]\n"
            "mysql_username: database_user_name"
            "passwd : database_passwd"
            "database: database_name"
            "mysql_max_connect_numbers: 20"
            "thread_pool_max_list: 1000"
            "thread_pool_nums: 4");
    }

    return;
}

bool webserver::webListen(std::string ip, int port){
    m_ip = ip;
    m_port = port;
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(m_listenfd == -1){
        ERROR("Create socket error %d", errno);
        LOG_CLOSE();
        return false;
    }

    struct sockaddr_in m_addr;
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    inet_pton(AF_INET, m_ip.c_str(), &m_addr.sin_addr);
    m_addr.sin_port = htons(m_port);
    //绑定
    int ret = bind(m_listenfd, (struct sockaddr*)&m_addr, sizeof(m_addr));
    if(ret == -1){
        ERROR("Socket bind error %d", ret);
        LOG_CLOSE();
        return false;
    }
    //监听
    ret = listen(m_listenfd, 100);
    if(ret == -1){
        ERROR("Listen error %d", ret);
        LOG_CLOSE();
        return false;
    }
    setnonblocking(m_listenfd);

    //注册listened连接事件
    struct epoll_event socketfd_read;
    socketfd_read.events = EPOLLIN;
    socketfd_read.data.fd = m_listenfd;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &socketfd_read);
    return true;
}


webserver::webserver():m_timerfdList(TIMESTEP, m_alarm_){
    //初始化日志
    LOG_INIT("logData", "log", 10);
    //创建epoll内核事件表
    m_epollfd = epoll_create(10);
    //读入设置
    configParser();
    //创建数据库池
    connection_pool::GetInstance()->init(mysql_ip, mysql_port, mysql_username, mysql_passwd, mysql_database, mysql_max_connect_numbers);
    //创建线程池
    m_threadPool = std::make_shared<threadPool<http>>();
    m_threadPool->init(connection_pool::GetInstance(), thread_pool_nums, thread_pool_max_list);
    
    //定时器
    //用timerfd系列函数注册读事件
    struct epoll_event m_alarm_read;
    m_alarm_read.events = EPOLLIN | EPOLLET;
    m_alarm_read.data.fd = m_alarm_;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_alarm_, &m_alarm_read);
    
};

void webserver::run(int n_timesteps = 5){
    //服务器主循环，持续等待epollfd的事件
    struct epoll_event events[MAX_EPOLL_EVENTS];
    INFO("webserver runing...");
    while(true){
        int nums = epoll_wait(m_epollfd, events, MAX_EPOLL_EVENTS, -1);
        if((nums < 0) && (errno != EINTR)){
            ERROR("epoll wait error: %d", errno);
            LOG_CLOSE();
            return;
        }
        bool alarm_event = false;
        for(int i = 0; i < nums; ++i){
            int m_fd = events[i].data.fd;
            //新连接
            if(m_fd == m_listenfd){
                //接受新连接
                struct sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                int new_con = accept(m_listenfd, (sockaddr *)&addr, &addrlen);
                if(new_con == -1){
                    ERROR("accept error, errno = %d",errno);
                    continue;
                }
                auto iter = m_data.find(new_con);
                if(iter == m_data.end()){
                    auto temp_http = std::make_shared<http>(addr);
                    //注册失败,关闭socket
                    if(!temp_http->init(m_epollfd, new_con)){
                        close(new_con);
                        ERROR("add epoll error,client[%s]", inet_ntoa(addr.sin_addr));
                        continue;
                    }
                    m_data[new_con] = temp_http;                   
                    INFO("accept new client[%s]", inet_ntoa(addr.sin_addr));
                } else {
                    m_data[new_con]->clearData();
                    m_data[new_con]->modfd(m_epollfd, new_con, EPOLLIN);
                }
                //添加定时器
                time_t now;
                time(&now);
                auto timer = std::make_shared<std::vector<long>>(2);
                (*timer.get())[0] = new_con;
                (*timer.get())[1] = now + TIMESTEP * n_timesteps;
                //m_timerList.add(timer);
                m_timerfdList.add(timer);

                m_data[new_con]->m_timer = timer;
                
                INFO("Connect socket[%d]", new_con);
            } 
            //定时器，接受到定时器读事件
            else if((m_fd == m_alarm_) && (events[i].events & EPOLLIN)) {
                uint64_t exp = 0;
                //读取，然后丢弃，让信号可以再次触发。
                int ret = read(m_alarm_, &(exp), sizeof(uint64_t));
                alarm_event = true;
            }
            //http连接的读事件
            else if(events[i].events & EPOLLIN){
                //更新定时器
                m_timerfdList.adjust(m_data[m_fd]->m_timer, n_timesteps);
                
                READ_STAT r_stat = m_data[m_fd]->read_once();
                switch (r_stat)
                {
                //读取的数据有误，直接丢弃，重新触发读事件
                case READ_STAT::BAD:
                {
                    INFO("Data error, client[%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
                    m_data[m_fd]->clearData();
                    //重新触发读事件
                    http::modfd(m_epollfd, m_fd, EPOLLIN);
                    continue;
                    break;
                }
                //对方关闭连接
                case READ_STAT::CLOSE:
                {
                    //关闭socket，从epoll内核事件表中移除，从m_data中移除
                    close(m_fd);
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_fd, 0);
                    //删除定时事件
                    m_timerfdList.delete_timer(m_data[m_fd]->m_timer);
                    INFO("Client close socket [%s]", inet_ntoa(m_data[m_fd]->get_addr().sin_addr));
                    m_data.erase(m_fd);
                    continue;
                    break;
                }
                case READ_STAT::OK:
                {
                    break;
                }
                default:
                    continue;
                    break;
                }
                //放入请求队列
                if(!m_threadPool->append(m_data[m_fd])){
                    ERROR("Add workList error. client[%d]", m_fd);
                    //清除数据，重新注册读事件
                    m_data[m_fd]->clearData();
                    http::modfd(m_epollfd, m_fd, EPOLLIN);
                    continue;
                }
                INFO("deal with client[%d]", m_fd);
            }
            //写事件
            else if(events[i].events & EPOLLOUT) {
                m_timerfdList.adjust(m_data[m_fd]->m_timer, n_timesteps);
                //write_once == false代表要关闭socket
                std::string ip = m_data[m_fd]->get_ip();
                if(ip == ""){
                    ip = inet_ntoa(m_data[m_fd]->get_addr().sin_addr);
                }
                INFO("Write to client[%d], url[%s], ip[%s]", m_fd, m_data[m_fd]->get_url().c_str(), ip.c_str());
                if(!m_data[m_fd]->write_once()){
                    INFO("Close socket, client[%d]", m_fd);
                    close(m_fd);

                    m_timerfdList.delete_timer(m_data[m_fd]->m_timer);
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_fd, 0);
                    m_data.erase(m_fd);
                } 
                
            }
            
        }
        //处理定时事件，把定时事件放到events事件执行完之后，防止信号触发把socket关闭了，但是events里边包含socket的读/写事件
        if(alarm_event){
            auto sockfds = m_timerfdList.tick();
            for(auto sfd : sockfds){
                close(sfd);
                epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sfd, 0);
                //关闭socket
                INFO("Close socket, client[%s]", inet_ntoa(m_data[sfd]->get_addr().sin_addr));
                m_data.erase(sfd);
            }
            alarm_event = false;
        }
    }
}

//注册静态资源
void webserver::addStaticSource(const std::string url, const std::string fileName){
    http::staticSource.insert({url, fileName});
}

//注册http的GET
void webserver::addGet(const std::string url, void (*func) (http_request&, http_response&, MYSQL*)){
    http::registerGet.insert({url, func});
}

//注册http的POST
void webserver::addPost(const std::string url, void (*func) (http_request&, http_response&, MYSQL*)){
    http::registerPost.insert({url, func});
}