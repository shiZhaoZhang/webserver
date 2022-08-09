#include "../httpRequestParser.cpp"
#include "string"
#include "iostream"

int main(){
    std::string request = "GET / HTTP/1.1\r\n"
    "User-Agent: PostmanRuntime/7.29.2\r\n"
    "Accept: */*\r\n"
    "Postman-Token: 16ac69d9-1271-4edf-9578-f87a1cdc3662\r\n"
    "Host: 127.0.0.1:1234\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Connection: keep-alive\r\n\r\n";
    http_request request_parser;
    request_parser.parser(request);
    std::cout << request << std::endl;
    std::cout <<"............." << std::endl;
    auto head = request_parser.get_head_params();
    for(auto iter = head.begin(); iter != head.end(); ++iter){
        std::cout << iter->first << " : " << iter->second << std::endl;
    }
    return 0;
}