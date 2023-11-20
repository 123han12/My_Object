
#ifndef HTTPSERVER
#define HTTPSERVER 
#define PORT 8081 ;

class HttpServer{
public:
    HttpServer(int port) ;  

    void InitServer() ;  // 设置当前进程忽略 SIGPIPE 中断信号
    void Loop() ; 

    ~HttpServer() ;  
private:
    int port_ ; 


}; 

#endif 