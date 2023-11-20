#ifndef CALLBACK
#define CALLBACK

#include "pch.h"  
#include "EndPoint.h" 
#include "ThreadPool.h"
#include <unistd.h>  


class CallBack : public Task 
{
public:
    CallBack(int sock) : sock_(sock) {} 


    Any HandlerRequest() override
    {
        LOG(INFO , "handler request begin") ; 
        std::shared_ptr<EndPoint> ep =  std::make_shared<EndPoint>(sock_) ; 

        ep->RecvHttpRequest() ; 

        if(!(ep->IsStop()) ) 
        {   
            LOG(INFO , "RECV NO ERROR , Begin Handler Request!") ; 
            ep->HandlerHttpRequest() ; 
            ep->BuildHttpResponse() ;
            ep->SendHttpResponse() ; 
            if(ep->IsStop()){
                LOG(WARNING, "Send Error, Stop Send Response");
            }
        }
        else {
            LOG(WARING , "Recv Error , Stop Handler Request!") ; 
        }

        close(sock_) ; 
        LOG(INFO , "handler request end") ; 
        
        return Any(nullptr) ; // 获取结果 
    }

    static void* callback(int sock)
    {
        LOG(INFO , "handler request begin") ; 
        std::shared_ptr<EndPoint> ep =  std::make_shared<EndPoint>(sock) ; 

        ep->RecvHttpRequest() ; 
        if(!(ep->IsStop()) ) 
        {
            LOG(INFO , "RECV NO ERROR , Begin Handler Request!") ; 
            ep->HandlerHttpRequest() ; 
            ep->BuildHttpResponse() ;
            ep->SendHttpResponse() ; 
            if(ep->IsStop()){
                LOG(WARNING, "Send Error, Stop Send Response");
            }
        }
        else {
            LOG(WARING , "Recv Error , Stop Handler Request!") ; 
        }

        close(sock) ; 
        LOG(INFO , "handler request end") ; 
        
       return nullptr ;  
    }

private:
    int sock_ ; 
};

#endif 