#include "httpRequestParser.h"
#include "iostream"
using namespace std;

int main(){
    http_request temp;
    string message = "GET / HTTP/1.1\r\nConnection: Close\r\nPostman-Token: ca219957-1620-43db-8b7c-791ca640e1e6\r\nHost: 127.0.0.1:12345\r\n\r\n";
    
    char request_2_c[] = "POST /?password=1010021&name=aerfa HTTP/1.1\r\n"
                    "Connection: Close\r\n"
                    "Content-Type: application/json\r\n"
                    "Postman-Token: d25eace0-a5d5-4f24-9d97-56686994842c\r\n"
                    "Host: 127.0.0.1:12345\r\n"
                    "Content-Length: 22\r\n";
                    "\r\n"
                    "Today is a raning day.";
    string request_2 = "POST /?password=1010021&name=aerfa HTTP/1.1\r\n"
                    "Connection: Close\r\n"
                    "Content-Type: application/json\r\n"
                    "Postman-Token: d25eace0-a5d5-4f24-9d97-56686994842c\r\n"
                    "Host: 127.0.0.1:12345\r\n"
                    "Content-Length: 22\r\n"
                    "\r\n"
                    "Today is a raning day.";
    std::string  str="POST /audiolibrary/music?ar=1595301089068&n=1p1 HTTP/1.1\r\n"
		"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-silverlight, application/x-shockwave-flash\r\n"
		"Referer: http://www.google.cn\r\n"
		"Accept-Language: zh-cn\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; TheWorld)\r\n"
		"content-length:27\r\n"
		"Host: www.google.cn\r\n"
		"Connection: Keep-Alive\r\n"
		"Cookie: PREF=ID=80a06da87be9ae3c:U=f7167333e2c3b714:NW=1:TM=1261551909:LM=1261551917:S=ybYcq2wpfefs4V9g; NID=31=ojj8d-IygaEtSxLgaJmqSjVhCspkviJrB6omjamNrSm8lZhKy_yMfO2M4QMRKcH1g0iQv9u\r\n"
		"\r\n"
		"hl=zh-CN&source=hp&q=domety";
    
    HTTP_CODE res = temp.parser(request_2);
    switch (res) {
        case HTTP_CODE::BAD_REQUEST :{
            cout << "bad request" << endl;
            break;
        }
        case HTTP_CODE::GET_REQUEST:{
            cout << "get request" << endl;
            break;
        }
        case HTTP_CODE::INTERNAL_ERROR : {
            cout << "internal error" << endl;
            break;
        }
        case HTTP_CODE::OPEN_REQUEST : {
            cout << "open request" << endl;
            break;
        }

    }
    if(HTTP_CODE::GET_REQUEST == temp.parser(request_2))
    {
        cout << "method " << temp.get_method() << endl;
        cout << "URL " << temp.get_URL() << endl;
        cout << "http version " << temp.get_http_version() << endl;
        cout << "body " << temp.get_body() << endl;
        cout << "URL_params : " << endl;
        auto url_params = temp.get_params();
        for(auto iter : url_params){
            cout << "\t" << iter.first << "=" << iter.second << endl; 
        }
        cout << "head_params : " << endl;
        auto head_params = temp.get_head_params();
        for(auto iter : head_params){
            cout << "\t" << iter.first << "=" << iter.second << endl; 
        }
    } else {
        cout << "input error: error_message = " << temp.get_error_message() << endl;
    }
    return 0;
}