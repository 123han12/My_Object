#ifndef PCH 
#define PCH 
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>


#include <string>
#include <iostream>
#include <chrono>
#include <ctime> 
#define INFO 1 
#define WARNING 2 
#define ERROR 3 
#define FATAL 4 

static void log(std::string level , std::string message , std::string file_name , int line )
{
    // 获得格式化的时间
    auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() ) ; 
    char timeString[100] ; 
    std::strftime(timeString , sizeof(timeString) , "%Y-%m-%d %H:%M:%S" , std::localtime(&currentTime) ) ; 

    std::cout << "[" << level << "] time is:" << timeString << " message: " << message << " file:" 
    << file_name << " line:" << line << std::endl << std::endl ; 
}

#define LOG(level , message)  log(#level , message , __FILE__ , __LINE__ ) 
#endif 

//获取参数
bool GetQueryString(std::string& query_string)
{
    bool result = false;
    std::string method ; 
    char* strptr = getenv("METHOD"); //获取请求方法
        
    method = strptr ; 

    if(method == "GET" ){ //GET方法通过环境变量获取参数
        
        query_string = getenv("QUERY_STRING") ; 
        result = true ; 
    }
    else if(method == "POST"){ //POST方法通过管道获取参数
        int content_length = atoi(getenv("CONTENT_LENGTH")) ; 
        char ch = 0;
        while(content_length){
            read(0, &ch, 1);
            query_string += ch;
            content_length--;
        }

        result = true;
    }

    return result;
}


//切割字符串
bool CutString(std::string& in, const std::string& sep, std::string& out1, std::string& out2)
{
    size_t pos = in.find(sep);
    if(pos != std::string::npos){
        out1 = in.substr(0, pos);
        out2 = in.substr(pos + sep.size() ); 
        return true;
    }
    return false;
}
int main()
{
    std::string query_string ; 
    GetQueryString(query_string) ; 


    std::string str1;
    std::string str2;
    CutString(query_string, "&", str1, str2) ;

    // std::cout << str1 << ' ' << str2 << std::endl ; 

    //以=为分隔符分别获取两个操作数的值
    std::string name1;
    std::string value1;
    CutString(str1, "=", name1, value1);
    std::string name2;
    std::string value2;
    CutString(str2, "=", name2, value2) ; 


    //处理数据
    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());
    
    std::cout<<"<html>" ; 
    std::cout<<"<head><meta charset=\"UTF-8\"></head>";
    std::cout<<"<body>";
    std::cout<<"<h3>"<<x<<" + "<<y<<" = "<<x+y<<"</h3>";
    std::cout<<"<h3>"<<x<<" - "<<y<<" = "<<x-y<<"</h3>";
    std::cout<<"<h3>"<<x<<" * "<<y<<" = "<<x*y<<"</h3>";
    std::cout<<"<h3>"<<x<<" / "<<y<<" = "<<x/y<<"</h3>"; //除0后cgi程序崩溃，属于异常退出
    std::cout<<"</body>";
    std::cout<<"</html>";


    return 0;
}


