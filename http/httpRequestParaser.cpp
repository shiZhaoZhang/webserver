#include "httpRequestParser.h"
const std::set<std::string> http_request::support_method{"GET", "POST", "PUT", "HEAD" , "DELETE", "OPTIONS", "TRACE", "CONNECT"};
const std::set<std::string> http_request::support_http_version{"HTTP/1.0", "HTTP/1.1"};

//解析主体：状态机
HTTP_CODE http_request::parser(const std::string &message){
    clear();
    int line_start = 0, line_end = 0;
    CHECK_STATE check_state = CHECK_STATE::CHECK_REQUESTLINE;
    while(line_start <= message.size()){
        //如果要获取报文主体，则一次读完，不用分行
        if(check_state != CHECK_STATE::CHECK_BODY){
            LINE_STATE line_state = get_line(message, line_start, line_end);
            if(line_state == LINE_STATE::LINE_BAD){
                error_message = "line bad";
                return HTTP_CODE::BAD_REQUEST;
            } else if(line_state == LINE_STATE::LINE_OPEN){
                return HTTP_CODE::OPEN_REQUEST;
            }
        }

        switch (check_state){
            case CHECK_STATE::CHECK_REQUESTLINE : {
                //请求行解析失败
                if(line_end < line_start + 2 || !request_parser(message.substr(line_start, line_end - line_start - 1))){
                    return HTTP_CODE::BAD_REQUEST;
                }
                line_start = line_end + 1;
                check_state = CHECK_STATE::CHECK_HEAD;
                break;
            }
            case CHECK_STATE::CHECK_HEAD : {
                //获取到了空行
                if(message.substr(line_start, line_end - line_start + 1) == "\r\n"){
                    check_state = CHECK_STATE::CHECK_BODY;
                    line_start = line_end + 1;
                    break;
                }
                //头部解析失败
                if(line_end < line_start + 2 || !head_parser(message.substr(line_start, line_end - line_start - 1))){
                    return HTTP_CODE::BAD_REQUEST;
                }
                line_start = line_end + 1;
                break;
            }
            case CHECK_STATE::CHECK_BODY : {
                //报文主体解析失败，不够长
                if(!body_parser(message.substr(line_start))){
                    return HTTP_CODE::OPEN_REQUEST;
                }

                return HTTP_CODE::GET_REQUEST;
            }
            default:{
                return HTTP_CODE::INTERNAL_ERROR;
            }
        }
    }
    //没有解析到主体完成就退出，且行没有解析失败，说明少接收信息。
    return HTTP_CODE::OPEN_REQUEST;
}

//分割行
//从line_start开始寻找'\r'，查看之后是不是'\n'，是的话就说明找到一行，不是且不是最后一个字符说明行错误。
LINE_STATE http_request::get_line(const std::string &message, int line_start, int &line_end){
    size_t mess_size = message.size();
    size_t temp = message.find("\r", line_start);
    if(temp != std::string::npos){
        if(temp + 1 == mess_size){
            return LINE_STATE::LINE_OPEN;
        } else if(message[temp + 1] == '\n'){
            line_end = temp + 1;
            return LINE_STATE::LINE_OK;
        }

        return LINE_STATE::LINE_BAD;
    }
    return LINE_STATE::LINE_OPEN;
}


//使用状态机解析请求行
bool http_request::request_parser(const std::string &request){
    size_t start = 0, end = request.find_first_of(" \t", start);
    enum class REQUEST{METHOD=0, URL, VERSION};
    REQUEST state = REQUEST::METHOD;
    while(end <= request.size()){
        switch (state)
        {
            case REQUEST::METHOD:{
                std::string temp_method = request.substr(start, end - start);
                //全大写
                std::transform(temp_method.begin(),temp_method.end(),temp_method.begin(),::toupper);
                //支持的方法
                if(support_method.find(temp_method) == support_method.end()){
                    //方法不支持
                    error_message = "method " + temp_method + " is not support";
                    return false;
                }
                m_method = temp_method;

                state = REQUEST::URL;
                start = end + 1;
                end = request.find_first_of(" \t", start);
                break;
            }   
            case REQUEST::URL : {
                std::string temp_url = request.substr(start, end - start);
                //因为url还会带一些字段，所以需要识别出来
                int index_ = temp_url.find('?');
                if(index_ != std::string::npos){
                    m_URL = temp_url.substr(0, index_);

                    std::string params = temp_url.substr(index_ + 1);
                    //解析参数，参数的形式是a=1&b=10&c=23
                    if(!params_parse(params)){
                        error_message = "URL params is error : " + params;
                        return false;
                    }   

                } else {
                    m_URL = temp_url;
                }
                
                state = REQUEST::VERSION;
                start = end + 1;
                end = request.size();
                break;
            }     
            case REQUEST::VERSION :{
                std::string temp_version = request.substr(start, end - start);
                
                //全大写
                transform(temp_version.begin(),temp_version.end(),temp_version.begin(),::toupper);
                //支持的方法
                if(support_http_version.find(temp_version) == support_http_version.end()){
                    //方法不支持
                    error_message = "http method " + temp_version + " is no support";
                    return false;
                }
                m_http_version = temp_version;
                return true;
            }
            default:
                end = std::string::npos;
                break;
        }
    }
    error_message = "request parser inner error";
    return false;
}
//解析参数，a=19&n=21312....
bool http_request::params_parse(const std::string &params){
    //首先判断输入合法性，多个&&连着，多个==连着
    if(params.find("&&") != std::string::npos || 
    params.find("==") != std::string::npos){

        return false;
    }

    //单个参数的起点和终点
    int param_start = 0;
    int param_end = params.find("&", param_start);
    param_end = (param_end == std::string::npos) ? params.size() : param_end;

    while(param_end <= params.size() && param_start < param_end){
        std::string param_ = params.substr(param_start, param_end - param_start);
        {
            int index_param_ = param_.find("=");
            //只有参数，没有值，说明请求行有问题
            if(index_param_ == std::string::npos || index_param_ == param_.size() - 1){
                return false;
            }
            m_params[param_.substr(0, index_param_)] = param_.substr(index_param_ + 1);
        }
        param_start = param_end + 1;
        param_end = params.find("&", param_start);
        param_end = (param_end == std::string::npos) ? params.size() : param_end;
    }
    return true;
}


//解析报文头部
bool http_request::head_parser(const std::string &header){
     size_t start = 0, end = header.find_first_of(":", start);
    if(end == 0 || end == std::string::npos){
        error_message = "header messgae error";
        return false;
    }
    //重复会覆盖

    //首部字段名称, Connect-Type
    std::string header_name = header.substr(start, end - start);
    //判断首部字段名词是不是有效的，依据：只有字母和-，只有一个-，-不可以在最后一位和第一位。
    {
        //全小写
        std::transform(header_name.begin(),header_name.end(),header_name.begin(),::tolower);
        //判断是不是只有字母和-
        if(header_name.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos){
            error_message = "http head parser: header name error";
            return false;
        }
        
        size_t temp_header_name_index = header_name.find('-');
        //-在最后和最前是非法的
        if(temp_header_name_index == 0 || (temp_header_name_index != std::string::npos && temp_header_name_index == header_name.size() - 1)){
            error_message = "http head parser: header name error";
            return false;
        }
        header_name[0] = header_name[0] - 32;
        if(temp_header_name_index != std::string::npos){
            //变大写
            ++temp_header_name_index;
            header_name[temp_header_name_index] = header_name[temp_header_name_index] - 32;
            //是否有多个-
            if(header_name.find('-', temp_header_name_index) != std::string::npos){
                error_message = "http head parser: header name error";
                return false;
            }
        }
        
    }
    //去除:后边的空格
    ++end;
    while(end < header.size() && header[end] == ' '){
        ++end;
    }
    m_head_params[header_name] = header.substr(end);
    return true;
}

bool http_request::body_parser(const std::string &body){
    //没找到实体主体大小的字段，没有实体
    if(m_head_params.find("Content-Length") == m_head_params.end()){
        return true;
    }
    int length = atoi(m_head_params["Content-Length"].c_str());
    //不够长
    if(body.size() <  length){
        error_message = "request body's length is not enough.";
        return false;
    }
    m_body = body.substr(0, length);
    return true;
}