//解析http请求报文，首先传入一个string，解析首部，接收报文主体。
//先接受一个完整请求行+首部，传入一个int类型的引用记录长度。报文的完整程度应该由接受的一层实现。

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include "string"
#include "map"
#include "algorithm"
#include "set"

//读取一行的数据状态：行完整，行不完整，行错误。
enum class LINE_STATE{LINE_OK = 0, LINE_OPEN, LINE_BAD};
//报文解析状态：请求行，头部，主体
enum class CHECK_STATE{CHECK_REQUESTLINE = 0, CHECK_HEAD, CHECK_BODY};
//HTTP报文解析结果：报文不完整，报文完整，报文错误, 内部错误（兜底）
enum class HTTP_CODE{OPEN_REQUEST = 0, GET_REQUEST, BAD_REQUEST, INTERNAL_ERROR};

class http_request{
public:
    http_request(){}
    ~http_request(){}
    //解析报文，传入需要解析的报文
    HTTP_CODE parser(const std::string &message);
public:
    //内部的错误信息存储在error_message中，通过get获取
    std::string get_error_message() const {
        return error_message;
    }
    std::string get_method() const {
        return m_method;
    }
    std::string get_URL() const {
        return m_URL;
    }
    std::map<std::string, std::string> get_params() const {
        return m_params;
    }
    std::string get_http_version() const {
        return m_http_version;
    }
    std::map<std::string, std::string> get_head_params(){
        return m_head_params;
    }
    std::string get_body() const {
        return m_body;
    }
    //清空
    void clear(){
        m_method.clear();
        m_URL.clear();
        m_params.clear();
        m_http_version.clear();
        m_head_params.clear();
        m_body.clear();
    }
private:
    //传入报文，解析开始和结束，函数负责对结束的地方赋值，返回LINE_STATE
    LINE_STATE get_line(const std::string &message, int line_start, int &line_end);
    //解析请求行
    bool request_parser(const std::string &request);
    //解析头部
    bool head_parser(const std::string &head);
    //解析主体
    bool body_parser(const std::string &body);
    
    //辅助函数
    bool params_parse(const std::string &params);
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
private:
    //内部错误信息
    std::string error_message;
    //支持的方法
    static const std::set<std::string> support_method;
    //支持的HTTP版本
    static const std::set<std::string> support_http_version;
};

#endif