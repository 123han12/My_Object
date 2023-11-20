#ifndef HTTPREQUEST
#define HTTPREQUEST 
#include <string>
#include <vector>
#include <unordered_map>
class HttpReuqest{
public:
    HttpReuqest(): content_length_(0) , cgi_(false) {} 
    ~HttpReuqest() {} 
public:
    // 读取到的请求报文
    std::string request_line_ ;  // 存储请求行
    std::vector<std::string> request_header_ ; // 请求报头
    std::string blank_ ; // 空行
    std::string request_body_ ; // 请求体

    // 分析后得到的结果
    std::string method_ ; // 请求方法
    std::string uri_ ; // 请求的标识符
    std::string version_ ; // 版本号
    std::unordered_map<std::string , std::string> header_kv_ ; // 请求报头中的键值对
    
    int content_length_ ; // 正文长度
    std::string path_ ; // 请求资源的路径
    std::string query_string_ ; // 请求报文中携带的参数

    bool cgi_ ; // 是否使用cgi


} ; 
#endif 