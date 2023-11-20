#ifndef TCPSERVER 
#define TCPSERVER 

#include "pch.h"
#define BACKLOG 5 


class TcpServer{
public:
    static TcpServer* GetInstance(int port)  ; 
    void InitServer()  ; // 初始化服务器 
    void Socket() ; 
    void Bind() ; 
    void Listen() ; 
    int Sock() ; 
    
    ~TcpServer() ; 
private:
    TcpServer(int port ) ;
    TcpServer(const TcpServer& ) = delete ; 
    TcpServer& operator=(const TcpServer&) = delete ; 
private:
    unsigned int port_ ; // 端口号
    int listen_sock_ ;  // 用来监听的套接字
    static TcpServer* svr_ ; // 单例指针 
} ;

#endif 