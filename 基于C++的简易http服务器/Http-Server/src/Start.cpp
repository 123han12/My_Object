#include "HttpServer.h"
#include <string>
#include <iostream>
#include <memory>

static void Usage(std::string proc ) 
{
    std::cout << "if you want to run httpservr please scanf: " << proc << " port" << std::endl ; 
}

int main(int argc , char* argv[] ) 
{

    if(argc != 2 )  // 如果参数的个数不是两个
    {
        Usage(argv[0] )  ; 
        exit(4) ; 
    }
    int port = atoi(argv[1] ) ;
    std::shared_ptr<HttpServer> svr(new HttpServer(port) ) ; 
    
    svr->InitServer() ; // 忽略 SIGPIPE 信号  
    svr->Loop() ; // 让服务器跑起来，不断的处理连接 

    return 0 ; 
}