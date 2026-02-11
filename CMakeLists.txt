cmake_minimum_required(VERSION 3.16)

project(QtServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core HttpServer)

qt_add_executable(QtServer main.cpp)

target_link_libraries(QtServer PRIVATE Qt6::Core Qt6::HttpServer)

# FIX: Only try to copy index.html if it actually exists in the source folder.
# This prevents Docker from failing when we optimize the build order.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index.html")
    configure_file(index.html index.html COPYONLY)
endif()