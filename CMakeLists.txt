cmake_minimum_required(VERSION 3.22)

project(redisc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_add_executable(redisc
    src/ChannelWindow.cpp
    src/ChannelWindow.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/RedisConnection.cpp
    src/RedisConnection.h
    src/SshTunnel.cpp
    src/SshTunnel.h
    src/main.cpp
)

target_link_libraries(redisc PRIVATE Qt6::Widgets Qt6::Network)
