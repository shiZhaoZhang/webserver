#include "httpResponse.h"

void http_response::base_request(const std::string code){ 
    //状态栏
    response_message_head = "HTTP/1.1 " + code + "\r\n";
    //消息报文
    {
        time_t rawTime;
        struct tm* timeInfo;
        char szTemp[36]={'\0'};
        time(&rawTime);
        timeInfo = gmtime(&rawTime);
        strftime(szTemp,sizeof(szTemp),"Data: %a, %d %b %Y %H:%M:%S GMT",timeInfo);
        response_message_head += szTemp;
    }
    response_message_head += "\r\n";   
    response_message_head += "Connection: keep-alive\r\n"; 
}

void http_response::add_ContentType(const std::string contentType){
    this->response_message_head += "Content-Type: " + contentType + "\r\n";
}

void http_response::add_Server(const std::string serverName){
    this->response_message_head += "Server: " + serverName + "\r\n";
}

void http_response::add_ContentLength(const long int length){
    if(length < 0){
        this->response_message_head += "Content-Length: 0\r\n";
    } else {
        this->response_message_head += "Content-Length: " + std::to_string(length) + "\r\n";
    }
}

void http_response::add_file(const std::string &fileName_){
    std::string filename = fileName_;
    this->response_message_body_file.reset(new mmapFile(filename));
    message_length = this->response_message_body_file->get_fileLength();
}

void http_response::add_message(const std::string &message){
    response_message_body_message = message;
    message_length = response_message_body_message.size();
}
//class mmapFile
mmapFile::mmapFile(const std::string fileName):m_fd(0), m_file(nullptr), m_filename(fileName){
    m_fd = open(m_filename.c_str(), O_RDONLY);
    //文件打开失败
    if(m_fd < 0) {
        char message[256] = {'\0'};
        sprintf(message, "Open file[%s] error", m_filename.c_str());
        LOG_ERROR("%s", message);
        return;
    }
    stat(m_filename.c_str(), &m_file_stat);
    m_file = mmap(nullptr, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
    if(m_file == MAP_FAILED){
        m_file = nullptr;
        char message[256] = {'\0'};
        sprintf(message, "Mmap file[%s] error", m_filename.c_str());
        LOG_ERROR("%s", message);
        close(m_fd);
        return;
    }
    close(m_fd);
}
mmapFile::~mmapFile(){
    if(m_file != nullptr)
    {
        if(munmap(m_file, m_file_stat.st_size) == -1){
            char message[256] = {'\0'};
            sprintf(message, "Munmap file[%s] error", m_filename);
            LOG_ERROR("%s", message);
        }
    }
}
long int mmapFile::get_fileLength(){
    if(m_file != nullptr){
        return m_file_stat.st_size;
    } else {
        return 0;
    }
}