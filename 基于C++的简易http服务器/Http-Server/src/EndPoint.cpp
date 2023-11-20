
#include "EndPoint.h"
#include "Util.h"
#include "pch.h" 

#include <sstream>
#include <algorithm>
#include <sys/stat.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <fcntl.h> 
#include <sys/sendfile.h>

#define SEP ": "

// 定义状态码
#define OK 200
#define BAD_REQUEST 400 
#define NOT_FOUND 404 
#define INTERNAL_SERVER 500  // internal_server 服务器内部错误

#define HTTP_VERSION "HTTP/1.0" 
#define LINE_END "\r\n"

#define PAGE_400 "400.html"
#define PAGE_404 "404.html"
#define PAGE_500 "500.html" 


// 定义路径资源
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html" 

EndPoint::EndPoint(int sock )
    : sock_(sock) , stop_(false)
     {}

EndPoint::~EndPoint() {} 

bool EndPoint::IsStop() // 提供给外部的接口，用于获取是否需要停止
{
    return stop_ ; 
}

void EndPoint::RecvHttpRequest()   // 读取客户端发来的HTTP请求
{   
    if( (!RecvHttpRequestLine() )  && ( !RecvHttpRequestHeader() ) ) { 
        ParseHttpRequestLine() ;
        ParseHttpReuqestHeader() ; 
        RecvHttpRequestBody() ; 
    }
}

void EndPoint::HandlerHttpRequest()   // 处理客户端发来的HTTP请求
{


    auto& code = http_Reponse_.status_code_ ; 
    
    auto& method = http_request_.method_ ; 

    if(method != "GET" && method != "POST" ) // 如果不是两种方法中的任意一种
    {
        LOG(WARING , "method is not right") ; 
        code = BAD_REQUEST ; 
        return ;  
    }
    if(method == "GET") 
    {   
        size_t pos = http_request_.uri_.find("?") ; 
#ifdef DEBUG 
        LOG(FATAL , http_request_.uri_) ; 
#endif 
        if(pos == std::string::npos ) 
        {
            http_request_.path_ = http_request_.uri_ ; 
        }
        else // uri中携带的有参数
        {
            // 存在参数就将uri_根据? 拆分为 资源路径和参数
            Util::CutString(http_request_.uri_ , http_request_.path_ , http_request_.query_string_ , "?") ;
            http_request_.cgi_ = true ; // 如果携带了参数，就需要cgi模式
        }
#ifdef DEBUG 
        LOG(FATAL , http_request_.uri_) ; 
        LOG(ERROR , http_request_.path_ ) ; 
        LOG(ERROR , http_request_.query_string_) ; 
#endif 

    }
    else if(method == "POST") // 这里为了提高可读性 
    {   
        http_request_.path_ = http_request_.uri_ ; 
        http_request_.cgi_ = true ; 
    }

    // 上面处理得到了 http_request_.path_ 和 http_request_.query_string 
    std::string path = http_request_.path_ ; 
    http_request_.path_ = WEB_ROOT ;
    http_request_.path_ += path ; 

    // 如果资源路径是一个目录的话，需要返回这个目录下的index.html文件
    if(http_request_.path_[http_request_.path_.size() - 1 ] == '/' ) 
    {   
        http_request_.path_ += HOME_PAGE ; // 路径加上 "index.html" 文件
    }

    // 判断客户端要访问的文件资源的属性，如果还是一个目录，则仍需要对资源路径进行处理。
    struct stat RFS ;  // resource file state    
    if(stat(http_request_.path_.c_str() , &RFS) == 0 )  
    {
        if(S_ISDIR(RFS.st_mode))  // 如果是一个目录的话
        {
            http_request_.path_ += '/' ; 
            http_request_.path_ += HOME_PAGE ; 
            stat(http_request_.path_.c_str(), &RFS ) ;  // 重新获取文件的信息
        }
        else if( (RFS.st_mode & S_IXUSR) || (RFS.st_mode & S_IXGRP) || (RFS.st_mode & S_IXOTH) )  // 如果文件是可执行的。
        {   
            http_request_.cgi_ = true ; 
        }
        http_Reponse_.size_ = RFS.st_size ; 
    }
    else { // 获取文件信息失败
        LOG(WARING , http_request_.path_ + " Not Found") ; 
        code = NOT_FOUND ; 
        return ; 
    }

#ifdef DEBUG 
   
    LOG(INFO , "begin Handler Request") ; 

#endif 

    // 处理后缀
    size_t pos = http_request_.path_.rfind(".") ; 
    if(pos == std::string::npos) 
    {
        http_Reponse_.suffix_ = ".html" ; 
    }
    else 
    {
        http_Reponse_.suffix_ = http_request_.path_.substr(pos) ;
    }

#ifdef DEBUG 
    LOG(INFO , "after parse httprequet , the cgi_ is " + std::to_string(http_request_.cgi_)) ; 
#endif 
    // 判断是否需要执行cgi程序
    if(http_request_.cgi_ ) 
    {
#ifdef DEBUG 
    LOG(INFO , "we entry ProcessCgi") ; 
#endif 
        code = ProcessCgi() ; 
    }
    else code = ProcessNoCgi() ;
#ifdef DEBUG 
    LOG(INFO , "end Hadnler Request") ; 
    LOG(INFO , "now the code is " + std::to_string(code) ) ;  
#endif 

 
}

void EndPoint::BuildHttpResponse()   // 构建将要发送给客户端的响应
{
    
#ifdef DEBUG 
    LOG(INFO , "begin BuildHttpRespoonse")  ; 
#endif 


    auto& status_line = http_Reponse_.status_line_ ; 
    int code = http_Reponse_.status_code_ ; 

    status_line += HTTP_VERSION ;
    status_line += " " ; 
    status_line += std::to_string(code) ; 
    status_line += " " ; 
    status_line += CodeToDesc(code) ; // 根据不同的状态码构建不同的响应
    status_line += LINE_END ;  // 每一行都是 \r\n 结尾。  

    // 构建响应报文头
    std::string path = WEB_ROOT ; 
    path += "/" ; 
    switch(code) {
        case 200 : 
            BuildOkResponse() ; 
            break ; 
        case 400 : 
            path += PAGE_400 ; 
            HandlerError(path) ; 
            break ; 
        case 404 :
            path += PAGE_404 ; 
            HandlerError(path) ;
            break ; 
        case 500 :
            path += PAGE_500 ; 
            HandlerError(path) ; 
            break ; 
        default :
            break ;
    }

#ifdef DEBUG 
    LOG(INFO , std::to_string(code) ) ;  
    LOG(INFO , "end BuildHttpRespoonse")  ; 
#endif 
}

bool EndPoint::SendHttpResponse()   // 发送 http响应给客户端
{   
    // 发送状态行
    Send(http_Reponse_.status_line_.c_str() , http_Reponse_.status_line_.size() ) ; 
    
    // 发送响应报头
    if(!stop_)
    {
        for(std::vector<std::string>::iterator iter = http_Reponse_.reponse_header_.begin() ; 
            iter != http_Reponse_.reponse_header_.end() ; ++iter 
        )
        {
            Send(iter->c_str() , iter->size() ) ; 
        }
    }
    // 发送空行
    if(!stop_) Send(http_Reponse_.blank_.c_str() , http_Reponse_.blank_.size()) ; 

    if(http_request_.cgi_ ) // 如果是cgi 模式，
    {
        if(!stop_)
        {
            auto& reponse_body = http_Reponse_.reponse_body_ ; 
            Send(reponse_body.c_str() , reponse_body.size() ) ;  // 发送对应的响应报文
        }
    }
    else {
        int fd = http_Reponse_.fd_ ; 
        if(!stop_)
        {
            sendfile(sock_ , fd , nullptr , http_Reponse_.size_ ) ; 
        }
        close(fd) ; 
    }
    return stop_ ; 
}


bool EndPoint::RecvHttpRequestLine(){ // 读取请求行
    auto& line = http_request_.request_line_ ; 
    if( Util::ReadLine( sock_ , line ) > 0 ) 
    {
        line.resize(line.size() - 1 ) ;  // 去掉读取过来的 \n
    }
    else {
        stop_ = true ; 
    }
    return stop_ ; 
}
bool EndPoint::RecvHttpRequestHeader() // 读取请求头和空行
{
    // 存储请求行的引用
    auto& re_vec = http_request_.request_header_ ; 
    std::string line ; 
    while(true)
    {
        line.clear() ; 
        if(Util::ReadLine(sock_ , line ) > 0 )  
        {
            if(line == "\n" )
            {
                http_request_.blank_ = line ; 
                break ; 
            } 
            line.resize(line.size() - 1 ) ; 
            re_vec.push_back(line) ; 
        }
        else {
            stop_ = true ;
            break ;  
        }  
    }

    return stop_ ; 
}
void EndPoint::ParseHttpRequestLine()  // 解析请求行
{
    std::string& line = http_request_.request_line_ ; 

    auto& method = http_request_.method_ ; 
    auto& uri = http_request_.uri_ ; 
    auto& version = http_request_.version_ ; 

    std::stringstream str(line) ; // 一个字符串流，读取字符串
    str >> method >> uri >> version ;  // 将方法，资源标识符，版本号读取到对应的字符串中

    
    std::transform(method.begin() , method.end() , method.begin() , toupper ) ; 

}
void EndPoint::ParseHttpReuqestHeader() // 解析请求报文
{
    std::string key ; 
    std::string value ; 
    auto& re_vec = http_request_.request_header_ ; 
    for(auto& item : re_vec ) 
    {
        if(Util::CutString(item , key , value , SEP ) ) 
        {
            http_request_.header_kv_.insert({key , value } ) ; 
        }   
        else {
            // 这里先不进行差错处理
        }
    }
}
bool EndPoint::RecvHttpRequestBody() // 读取请求正文
{
    // 需要先判断其是否携带有请求正文？
    if(!IsNeedReadHttpRequestBody() ) return stop_ ; 
    //根据 http_request_.content_length_ 从 sock 中读取数据
    int len = http_request_.content_length_ ; 
    auto& body = http_request_.request_body_ ; 
    char ch ;  
    while( len > 0 )
    {
        int size = recv(sock_ , &ch , 1 , 0 ) ; 
        if(size > 0 ) 
        {
            body.push_back(ch) ;
            len -- ;  
        } 
        else  
        {
            stop_ = true ; 
            break ; 
        }
    }
    return stop_ ; 
}


bool EndPoint::IsNeedReadHttpRequestBody()
{
    auto& method = http_request_.method_ ; 
    if(method == "GET" ) 
    {
        return false ; 
    }
    auto& head_kv = http_request_.header_kv_ ; 
    std::unordered_map<std::string , std::string>::iterator iter = head_kv.find("Content-Length") ; 
    if(iter != head_kv.end() ) 
    {
        http_request_.content_length_ = atoi(iter->second.c_str() ) ; 
        return true ;
    }
    return false ; // 当 iter == head_kv.end()的时候，这个才会成立
}

void EndPoint::Send(const char* str , int size ) 
{
    int total = size ; 
    int len = 0 ;
    while(len < total ) 
    {
        int size = send(sock_ , str + len , total - len  , 0 ) ;
        if(size <= 0 ) 
        {
            stop_ = true ; 
            return ; 
        }
        len += size ; 
        
    }
}


int EndPoint::ProcessCgi(){ // 进行cgi处理 ，返回值是状态码
    int code = OK ; 
    
    auto& bin = http_request_.path_ ; 
    auto& method = http_request_.method_ ;

    // 需要给CGI程序传递的参数
    auto& query_string = http_request_.query_string_ ; 
    auto& request_body = http_request_.request_body_ ; 

    
    int content_length = http_request_.content_length_ ; // 请求正文长度
    auto& response_body = http_Reponse_.reponse_body_ ; // 处理结果放回到响应报文体中

    int input[2] ;
    if(pipe(input) < 0 ) // 创建父进程输入管道错误
    {
        LOG(ERROR , "pipe input error") ; 
        code = INTERNAL_SERVER ;  // 设置状态码
        return code ; 
    }

    int output[2] ; 
    if(pipe(output) < 0 ) //创建父进程输出管道错误
    {
        LOG(ERROR , "pipe output error" ) ; 
        code = INTERNAL_SERVER ; 
        return code ; 
    }
#ifdef DEBUG 
    LOG(INFO , "now the code is " + std::to_string(code) ) ;  
#endif

    pid_t pid = fork() ;  // 创建子进程

    if(pid == 0 ) // 如果这里是子进程。 
    {
        
        close(input[0] ) ; 
        close(output[1] ) ; 

        // 将请求方法通过环境变量传递参数
        std::string method_env = "METHOD=" ; 
        std::string query_env = "QUERY_STRING=" ;
        std::string content_length_env = "CONTENT_LENGTH=" ; 

        method_env += method ; 
        putenv(const_cast<char*>(method_env.c_str()) ) ;  // 这里先不考虑失败

        // const_cast<char*>(method_env.c_str())

        if(method == "GET") 
        {    
            query_env += query_string ; 
#ifdef DEBUG 
            LOG(FATAL , query_env) ; 
#endif 
            putenv(const_cast<char*>(query_env.c_str()) ) ;  // 这里先不考虑失败             
            LOG(INFO , "GET Method , Add Query_String env" ) ; 
        }
        else if(method == "POST") 
        {
            content_length_env += std::to_string(content_length) ; 
            putenv( const_cast<char*>(content_length_env.c_str())  ); 
            
            // const_cast<char*>(content_length_env.c_str())
            LOG(INFO , "POST Method , Add Content-Length env" ) ;  
        }
#ifdef DEBUG
        LOG(WARING , "begin swap file......") ; 
        LOG(WARING , bin.c_str() ) ; 
#endif   
        dup2(output[0] , STDIN_FILENO) ;    
        dup2(input[1] , STDOUT_FILENO ) ;  

        // 进行进程的替换
        execl(bin.c_str() , bin.c_str() , NULL ) ; 
#ifdef DEBUG
        LOG(WARING , "file exchange is failed....") ;  
#endif

       // exit(1) ; // 替换失败，这个子进程就直接退出
    }
    else if(pid < 0 ) 
    {
        LOG(ERROR , "fork error") ; 
        code = INTERNAL_SERVER ; 
        return code ; 
    }
    else if (pid > 0 ){ // 父进程执行逻辑
        close(input[1]) ; 
        close(output[0]) ; 

        if(method == "POST") // 如果是post 方法，就将请求正文传递到管道中
        {
            const char* start = request_body.c_str() ; 
            int total = 0 , size = 0 ; 
            while(total < content_length && (size = write(output[1] , start + total , request_body.size() - total ) > 0 ))
            {
                total += size ; 
            }   
        }

        char ch = 0 ; 

        while(read(input[0] , &ch , 1 ) > 0 ) 
        {
            response_body.push_back(ch) ; 
        }
#ifdef DEBUG 
        LOG(INFO , "now the code is " + std::to_string(code) ) ; 
        LOG(INFO , "cgi progress id is " + std::to_string(pid) ) ;  
#endif

        // 等待CGI程序退出
        int status = 0 ;
        pid_t ret = waitpid(pid , &status , 0 ) ; // 如果cgi程序没有执行完毕就阻塞
#ifdef DEBUG 
        LOG(INFO , "waitpid return  " + std::to_string(ret) ) ;   
#endif        
        if(ret == pid ) 
        {
            if(WIFEXITED(status) )  // 如果正常退出
            {
                if(WEXITSTATUS(status) == 0 )  // 取得退出的时候的状态码等于0的话
                {
                    LOG(INFO , "CGI program exits normally with correct results") ; 
                    code = OK ; 
                }
                else {
                    LOG(INFO , "CGI program exits normally with incorrect results") ; 
                    code = BAD_REQUEST ; 
                }    
            }
            else {
                LOG(INFO , "CGI program exits abnormally") ; 
                code = INTERNAL_SERVER ; 
            }
        }
#ifdef DEBUG 
        LOG(INFO , "now the code is " + std::to_string(code) ) ; 
        // code = OK ;
#endif
        close(input[0] ); 
        close(output[1] ); 
    }
    return code ; // 返回状态码
}
int EndPoint::ProcessNoCgi() // 不进行 cgi处理
{
    http_Reponse_.fd_ = open(http_request_.path_.c_str() , O_RDONLY ); 
    if(http_Reponse_.fd_ < 0 ) 
    {
        return INTERNAL_SERVER ; // 返回内部错误。 
    }
    return OK ; 
}

const char* EndPoint::CodeToDesc(int code )
{
    switch(code)
    {
        case 200 :
            return "OK" ;
        case 400 : 
            return "Bad Request" ; 
        case 404 :
            return "Not Found" ; 
        case 500 : 
            return "Internal Server Error" ; 
        default :
            return "undefined state!";  
    }
    
}


void EndPoint::BuildOkResponse() 
{   
    std::string content_type = "Content-Type: " ; 
    content_type += SuffixToDesc(http_Reponse_.suffix_ ) ; 
    content_type += LINE_END ; 
    http_Reponse_.reponse_header_.push_back(content_type) ; 
    
    std::string content_length = "Content-Length: " ; 
    if(http_request_.cgi_ ) // 如果是cgi 模式
    {
        content_length += std::to_string(http_Reponse_.reponse_body_.size()) ;  
    }
    else {
        content_length += std::to_string(http_Reponse_.size_ ) ; 
    }
    content_length += LINE_END ; 
    http_Reponse_.reponse_header_.push_back(content_length) ; 
}

void EndPoint::HandlerError(std::string path) 
{
    http_request_.cgi_ = false ; 
    http_Reponse_.fd_ = open(path.c_str() , O_RDONLY) ; 
    if(http_Reponse_.fd_ > 0 )
    {
        struct stat st ; 
        stat(path.c_str() , &st ) ; 
        std::string content_type = "Content-Type:text/html" ;
        content_type += LINE_END ; 
        http_Reponse_.reponse_header_.push_back(content_type) ; 

        std::string content_length = "Content-Length:" ;
        content_length += std::to_string(st.st_size) ;  
        content_length += LINE_END ; 
        http_Reponse_.reponse_header_.push_back(content_length) ; 

        http_Reponse_.size_ = st.st_size ;  // 重新设置响应文件的大小
    }

}
std::string EndPoint::SuffixToDesc(const  std::string& suffix )  // 根据文件后缀名，返回对应的类型
{
    static std::unordered_map<std::string , std::string > suffix_to_desc = {
        {".html" , "text/html"} ,
        {".css" , "test/css"} ,
        {".js" , "application/x-javascript"} , 
        {".jpg" , "application/x-jpg"} , 
        {".xml" , "text/xml"} 
    } ; 
    std::unordered_map<std::string , std::string>::iterator iter = suffix_to_desc.find(suffix) ; 
    if(iter == suffix_to_desc.end() ) // 每找到这种类型的话，直接返回 text/html 
    {
        return std::string("text/html") ; 
    } 
    return iter->second ; // 返回其第二个元素
} 


