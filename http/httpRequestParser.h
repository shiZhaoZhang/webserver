//解析http请求报文，首先传入一个string，解析首部，接收报文主体。
//先接受一个完整请求行+首部，传入一个int类型的引用记录长度。报文的完整程度应该由接受的一层实现。
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include "string"
#include "map"
//读取一行的数据状态：行完整，行不完整，行错误。
enum class LINE_STATE{LINE_OK = 0, LINE_OPEN, LINE_BAD};
//报文解析状态：请求行，头部，主体
enum class CHECK_STATE{CHECK_REQUESTLINE = 0, CHECK_HEAD, CHECK_BODY};
//HTTP报文解析结果：报文不完整，报文完整，报文错误
enum class HTTP_CODE{OPEN_REQUEST = 0, GET_REQUEST, BAD_REQUEST};
class http_request{
public:

private:
    //解析一行内容

private:
    //方法
    std::string m_method;
    //URL
    std::string m_URL;
    //参数
    std::map<std::string, std::string> m_params;
    //HTTP版本
    std::string m_http_version;
    //头部字段
    std::map<std::string, std::string> m_head_params;
    //主体
    std::string m_body;
};

#endif