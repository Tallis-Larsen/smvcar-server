cmake_minimum_required(VERSION 3.16)
project(smvcar-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automates the Qt meta object compiler
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core HttpServer WebSockets)

# The CONFIGURE_DEPENDS flag requires cmake to re-glob on every build
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "include/*.h") # Header files need to be included so that the QT MOC picks up on them
add_executable(smvcar-server ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(smvcar-server PRIVATE include)
target_link_libraries(smvcar-server PRIVATE Qt6::Core Qt6::HttpServer Qt6::WebSockets)