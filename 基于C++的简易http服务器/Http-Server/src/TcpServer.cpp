#include "TcpServer.h"
#include <thread>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>  // struct sockaddr_in 位于的头文件
#include <string.h>
#include <unistd.h> // close()所位于的头文件


TcpServer::TcpServer(int port ) 
:port_(port) , listen_sock_(-1) 
{
    InitServer() ; 
}

TcpServer* TcpServer::GetInstance(int port)  
{
    static TcpServer obj(port) ;  // 天然的线程安全
    return &obj ;  // 返回这个obj对象的指针
}
void TcpServer::InitServer() // 初始化服务器
{
    Socket() ; // 创建套接字 
    Bind() ;  // 绑定套接字
    Listen() ; // 监听套接字
    LOG(INFO , "tcpserver init.....success!" ) ; 
}
void TcpServer::Socket(){
    listen_sock_ = socket(AF_INET , SOCK_STREAM ,  0 ) ; 
    if(listen_sock_ < 0 ) 
    {
        LOG(FATAL , "socket error!") ; 
        exit(1) ; 
    } 
    // 设置端口复用
    int opt = 1 ; 
    // 关闭之后，不用等待2 * time_wait 的时间就能直接复用
    setsockopt(listen_sock_ , SOL_SOCKET , SO_REUSEADDR , &opt , sizeof(opt ) ) ; 
    LOG(INFO , "create socket sucess......") ; 
}
void TcpServer::Bind()
{
    struct sockaddr_in local ; 
    memset( &local , 0 , sizeof(local) ) ; 
    local.sin_family = AF_INET ;
    local.sin_port = htons(port_) ;
    local.sin_addr.s_addr = INADDR_ANY ;  
    
    if(bind(listen_sock_ , reinterpret_cast<sockaddr*>(&local) , sizeof (local) ) !=0 ) 
    {
        // 绑定不成功
        LOG(FATAL , strerror(errno) ) ;  
        /*
            strerror() : Return a string describing the meaning of the errno code in ERRNUM 
        */
        exit(2) ; 
    } 
    LOG(INFO , "bind is success.......") ; 
}

void TcpServer::Listen()
{
    if(listen(listen_sock_ , 10 ) != 0 )
    {
        LOG(FATAL , strerror(errno) ) ;
        exit(3) ;  
    }
    LOG(INFO , "listen is sucess......") ; 
}
int TcpServer::Sock()
{
    return listen_sock_ ; 
}
TcpServer::~TcpServer()
{
    if(listen_sock_ >= 0 ) close(listen_sock_ ) ;
}

TcpServer* TcpServer::svr_ = nullptr ;