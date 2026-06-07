cmake_minimum_required(VERSION 3.22)

project(redisc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(CMakePackageConfigHelpers)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui Test)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed)
find_package(pybind11 CONFIG REQUIRED)

set(Python314t_INCLUDE_DIR "${Python3_INCLUDE_DIRS}")
set(Python314t_LIBRARY "${Python3_LIBRARIES}")
set(Python314t_EXECUTABLE "${Python3_EXECUTABLE}")
set(Python3_TARGET_VERSION "${Python3_VERSION}")
if(NOT TARGET Python314t::Python)
    add_library(Python314t::Python INTERFACE IMPORTED)
    target_link_libraries(Python314t::Python INTERFACE Python3::Python)
endif()

set(pybind11_SOURCE_DIR "/usr")
add_subdirectory(qtpyt/src)
target_include_directories(qtpyt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/qtpyt/include")

qt_add_executable(redisc
    src/ChannelWindow.cpp
    src/ChannelWindow.h
    src/JsonTreeWindow.cpp
    src/JsonTreeWindow.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/PythonClient.cpp
    src/PythonClient.h
    src/RedisConnection.cpp
    src/RedisConnection.h
    src/SshTunnel.cpp
    src/SshTunnel.h
    src/main.cpp
    resources.qrc
)

target_include_directories(redisc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/qtpyt/include")
target_link_libraries(redisc PRIVATE Qt6::Widgets Qt6::Network qtpyt)

add_custom_command(TARGET redisc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/themes"
        "$<TARGET_FILE_DIR:redisc>/themes"
)

enable_testing()

qt_add_executable(redisc_python_api_tests
    tests/PythonClientTest.cpp
    src/PythonClient.cpp
    src/PythonClient.h
)

target_include_directories(redisc_python_api_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/qtpyt/include"
)
target_link_libraries(redisc_python_api_tests PRIVATE Qt6::Test Qt6::Core qtpyt)

add_test(NAME python_api COMMAND redisc_python_api_tests)
