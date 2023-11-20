#ifndef PCH 
#define PCH 

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