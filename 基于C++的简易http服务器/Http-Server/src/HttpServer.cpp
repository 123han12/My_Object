#include "HttpServer.h"
#include "TcpServer.h"
#include "pch.h" 
#include "ThreadPool.h"
#include "CallBack.h"

#include <thread> 
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <string> 
#include <signal.h> 

HttpServer::HttpServer(int port) 
: port_(port) 
{} 


void HttpServer::InitServer()
{
    signal(SIGPIPE , SIG_IGN) ;  // 服务器进程忽略因为连接中断导致的中断信号SIGPIPE 
}

void HttpServer::Loop()
{
    LOG(INFO , "loop begin......") ;
    TcpServer* tptr = TcpServer::GetInstance(port_) ;  
    int sock = tptr->Sock() ;  // 获取监听的套接字
    
    // 进行线程池初始化
    ThreadPool pool ; 
    pool.setMode(PoolMode::MODE_CACHED) ; 
    pool.setInitThreadSize(10) ; 
    pool.start() ; 
    Result res ; 

    while(true)
    {
        struct sockaddr_in peer ; 
        memset(&peer , 0 , sizeof(peer ) ) ; 
        socklen_t len = sizeof(peer) ; 
        int cfd = accept(sock , reinterpret_cast<sockaddr*>(&peer) , &len ) ; // 获取连接

        if(cfd == -1)  continue ;               

        char buf[1024] ; 
        inet_ntop(AF_INET , &(peer.sin_addr.s_addr) , buf , sizeof(buf) );

        int client_port = ntohs(peer.sin_port) ; 
        std::string message ; 
        message = message + "get a new link ip:" + buf + " port: " + std::to_string(client_port)  ;  
        LOG(INFO , message ) ; 
        // 将返回的结果存储于res 中 
        
        pool.submitTask( std::make_shared<CallBack>(cfd) , &res ) ;  

        // std::thread th(&CallBack::callback , cfd );
         
        // th.detach() ; 
    }
}
HttpServer::~HttpServer() {} 