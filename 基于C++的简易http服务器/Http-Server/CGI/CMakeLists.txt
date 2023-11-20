cmake_minimum_required(VERSION 3.0.0)

project(cgi) 

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ) 

add_definitions(-DDEBUG) 

#指定输出文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin/wwwroot/ )  

#指定头文件对应的目录
include_directories(${PROJECT_SOURCE_DIR}/../include )  

add_executable(test_cgi  ${SRC} )     
 