cmake_minimum_required(VERSION 3.0.0)

project(ThreadPool) 

# 定义 DEBUG 宏
# add_definitions(-DDEBUG) #注意这里要加上-D 

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ) 

#指定输出文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin ) 



#指定头文件对应的目录
include_directories(${PROJECT_SOURCE_DIR}/include )  

add_executable(httpserver  ${SRC} )     
 