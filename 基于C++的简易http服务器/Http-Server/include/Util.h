#ifndef UTIL 
#define UTIL 
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

class Util{
public:
    static int ReadLine(int sock , std::string& line ) // 读取一行的数据。
    {
        char ch = 'X';
        while(ch != '\n') 
        {
            ssize_t size = recv(sock , &ch , 1 , 0 ) ;
            if(size > 0 ) 
            {
                if(ch == '\r')
                {
                    recv(sock , &ch , 1 , MSG_PEEK) ; 
                    if(ch == '\n') 
                    {
                        recv(sock , &ch , 1 ,  0 ) ;  
                    }
                    else ch = '\n' ; 
                }
                line.push_back(ch) ; 
            }
            else if(size == 0 ) 
            {
                return 0 ;
            }
            else {
                return -1 ; 
            }
        }
        return line.size() ; 
    }
    
    static bool CutString(std::string old , std::string &out1 , std::string& out2 , std::string splic ) 
    {
        size_t iter = old.find(splic) ; 
        if(iter == std::string::npos ) return false ;
        out1 = old.substr(0 , iter ) ;  // 从0开始提取iter个。
        out2 = old.substr(iter + splic.size() ) ; // 省略第二个参数的话，提取从起始位置到最后位置的所有元素。
        return true ;   
    }

}; 

#endif 