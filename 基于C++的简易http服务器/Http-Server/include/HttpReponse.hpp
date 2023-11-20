#ifndef HTTPREPONSE
#define HTTPREPONSE 

#include <string>
#include <vector> 
#define OK 200 
#define LINE_END "\r\n"
class HttpReponse{
public:
    HttpReponse() 
    : blank_(LINE_END) , 
    status_code_(OK) , 
    fd_(-1) , 
    size_(0) 
    {}
    ~HttpReponse() {}
public:
    // 响应的内容
    std::string status_line_ ; // 状态行
    std::vector<std::string> reponse_header_ ; // 响应头
    std::string blank_ ; //空行
    std::string reponse_body_ ;  // 响应正文

    int status_code_ ; // 状态码
    int fd_ ;  // 响应文件的fd
    int size_ ; // 响应文件的大小
    std::string suffix_ ; // 响应文件的后缀。 
}; 

#endif 