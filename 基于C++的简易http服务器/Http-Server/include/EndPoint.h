#ifndef ENDPOINT
#define ENDPOINT 
#include "HttpReponse.hpp"
#include "HttpRequest.hpp"

class EndPoint{
public:
    EndPoint(int sock ) ; 
    ~EndPoint() ; 

    bool IsStop() ; 
    void RecvHttpRequest() ;  // 读取客户端发来的HTTP请求

    void HandlerHttpRequest() ;  // 处理客户端发来的HTTP请求

    void BuildHttpResponse() ; 
    
    bool SendHttpResponse()  ; // 发送 http响应给客户端
    
private:

    bool RecvHttpRequestLine() ; 
    bool RecvHttpRequestHeader() ; // 读取请求头和空行

    void ParseHttpRequestLine()   ;  // 解析请求行
    void ParseHttpReuqestHeader() ;  // 解析请求报文

    bool RecvHttpRequestBody() ;  // 读取请求正文

private:

    bool IsNeedReadHttpRequestBody() ; 
    void Send(const char* str , int size ) ; 

private:
    int ProcessCgi() ;  // 进行cgi处理 ，返回值是状态码
    int ProcessNoCgi() ; // 不进行 cgi处理

    const char* CodeToDesc(int code ) ; 

    void BuildOkResponse()  ; 

    void HandlerError(std::string path)  ; 
    std::string SuffixToDesc(const  std::string& suffix ) ;  // 根据文件后缀名，返回对应的类型
private:
    int sock_ ;  // 通信的套接字
    HttpReuqest http_request_ ; // 客户端发送过来的请求
    HttpReponse http_Reponse_ ; // 将会发送的响应 
    bool stop_ ; // 是否停止本次处理 
} ;

#endif 