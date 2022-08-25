#include <server.h>
#include "unistd.h"
#include "iostream"
using namespace std;
void registFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql);
void loginFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql);
void userLoginFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql);
void userRegisterFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql);

int main(int argc, char* argv[]){
    if(argc < 3){
        printf("need filename ip-address port\n");
        return 1;
    }
    //ip地址
    char *ip = argv[1];
    //端口
    int port = atoi(argv[2]);

    webserver web;
    if(!web.openhttps("ssl/cert.pem", "ssl/privkey.pem", "123456")){
        ERROR("web openhttps error");
    }
    if(!web.webListen(ip, port)){
        sleep(1);
        return 1;
    } 
    //注册url
    web.addPost("/0", registFunc);
    web.addPost("/1", loginFunc);
    web.addGet("/2CGISQL.cgi", userLoginFunc);
    web.addGet("/3CGISQL.cgi", userRegisterFunc);
    web.addStaticSource("/5", "./root/picture.html");
    web.addStaticSource("/6", "./root/video.html");
    web.addStaticSource("/7", "./root/fans.html");
    web.addStaticSource("/8", "./root/JComputPhys.435.110240.pdf");
    web.addStaticSource("/996", "./root/996.ICU.html");
    web.run(5);
}

void registFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql){
    LOG_INFO("[POST] url[%s]", m_request.get_URL().c_str());
    m_response.base_request("200 OK");
    m_response.add_Server("Bowu.server v1.0");
    m_response.add_file("./root/register.html");
    m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
    m_response.end_response_message_head();
}

void loginFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql){
    LOG_INFO("[POST] url[%s]", m_request.get_URL().c_str());
    m_response.base_request("200 OK");
    m_response.add_Server("Bowu.server v1.0");
    m_response.add_file("./root/login.html");
    m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
    m_response.end_response_message_head();
}

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

void userRegisterFunc(http_request& m_request, http_response& m_response, MYSQL * m_mysql){
    auto params = m_request.get_params();
    if(params.find("user") == params.end() || params.find("password") == params.end())
    {
        m_response.base_request("400 Bad Request");
        m_response.add_Server("Bowu.server v1.0");
        m_response.add_ContentLength(0);
        m_response.end_response_message_head();
    } else {
        m_response.base_request("200 OK");
        m_response.add_Server("Bowu.server v1.0");

        auto user_name = params["user"];
        auto passwd = SHA512(params["password"].c_str());
        //查看用户是否已经存在
        std::string res = SearchUserAndPasswd(user_name.c_str(), m_mysql);
        if(res != "" || !InsertUser(user_name.c_str(), passwd.c_str(), m_mysql)){
            m_response.add_file("./root/registerError.html");
            m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
        } else {
            m_response.add_file("./root/login.html");
            m_response.add_ContentLength(m_response.response_message_body_file->get_fileLength());
        }
        m_response.end_response_message_head();
    }
     
    return;
}
