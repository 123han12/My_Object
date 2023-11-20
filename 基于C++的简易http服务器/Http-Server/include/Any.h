
#ifndef ANY 
#define ANY 
#include "public.h"
class Any {
public: 

    // 模版构造函数 ， 可以接受任意类型的数据
    template<typename T> 
    Any(T data): base_( std::make_unique< Derive<T> > (data))
    {}

    Any() = default ;
    ~Any() = default ; 
    Any(const Any&) = delete ; 
    Any& operator=(const Any&) = delete ; 
    Any(Any&&) = default ; 
    Any& operator=(Any&&) = default ;  


    // 这个方法是将 Any类型中的数据提取出来
    template<typename T >
    T cast_()
    {
        // 怎么从base_中找到它所指向的派生类Drive对象，从其中提取出来数据。
                                                     
        // 基类指针转为派生类指针 , 支持RTTI 
        Derive<T>* pd = dynamic_cast< Derive<T>* >(base_.get()) ; 
       
        // std::cout << pd << std::endl ; 
        // getchar() ;  
        if (pd == nullptr ) // 为什么会走到这里呢？？
        {
            throw "type is unmatch" ;  
           
        }     // 当 cast 中的T 和 Derive 中的 T 不同的时候，会出现这种情况
        return pd->data_ ;  
    }

private:
    class Base {
    public:
        virtual ~Base() = default ; // 如果派生类对象是在堆上创建的，最好将析构函数设置为虚函数 
        
    } ; 

    // 派生类类型
    template<typename T> 
    class Derive : public Base
    {
    public :
        Derive(T data) : data_(data) {} 

        T data_ ;  
    };
    
private: 
    std::unique_ptr<Base> base_ ; // 定义一个基类指针。
};

#endif 