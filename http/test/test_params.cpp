//测试分割字符串
#include "string"
#include "iostream"
#include "map"
#include "algorithm"
#include "set"

using namespace std;
const std::set<std::string> support_method{"GET", "POST", "PUT", "HEAD" , "DELETE", "OPTIONS", "TRACE", "CONNECT"};
const std::set<std::string> support_http_version{"HTTP/1.0", "HTTP/1.1"};

string m_method;
string m_URL;
string m_http_version;
string error_message;

map<string, string> m_params;
map<string, string> m_head_params;
bool params_parse(const std::string &params){
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
bool request_parser(const std::string &request){
    size_t start = 0, end = request.find_first_of(" \t", start);
    enum class REQUEST{METHOD=0, URL, VERSION};
    REQUEST state = REQUEST::METHOD;
    while(end <= request.size()){
        switch (state)
        {
            case REQUEST::METHOD:{
                std::string temp_method = request.substr(start, end - start);
                //全大写
                transform(temp_method.begin(),temp_method.end(),temp_method.begin(),::toupper);
                //支持的方法
                if(support_method.find(temp_method) == support_method.end()){
                    //方法不支持
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
                    if(!params_parse(params))   return false;

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
    return false;
}
//解析报文头部
bool head_parser(const std::string &header){
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
int main(int argc, char* argv[]){

    string test_params = argv[1];//"a=asbduib&sadjbj=22e12e&&&&";
    /*cout << ": " << test_params.substr(test_params.size()) << endl;
    cout << test_params << endl;
    if(!request_parser(test_params)){
        cout << "input error" << endl;
    } else {
        cout << "method = " << m_method << endl;
        cout << "URL = " << m_URL << endl;
        cout << "version = " << m_http_version << endl;
        cout << "params : " << endl;
        for(auto iter : m_params){
            cout << iter.first << ":" << iter.second << endl;
        }
    }*/
    if(!head_parser(test_params)){
        cout << "input error" << endl;
    } else {
        for(auto iter : m_head_params){
            cout << iter.first << "=" << iter.second << endl;
        }
    }
    return 0;
}