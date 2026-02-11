cmake_minimum_required(VERSION 4.0)

project(QtServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 Core and HttpServer components
find_package(Qt6 REQUIRED COMPONENTS Core HttpServer)

# Define the executable
qt_add_executable(QtServer main.cpp)

# Link the libraries
target_link_libraries(QtServer PRIVATE Qt6::Core Qt6::HttpServer)

# Ensure the index.html is copied to the build directory (optional for local testing)
configure_file(index.html index.html COPYONLY)