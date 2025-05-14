#############
## pathstr ##
#############
cmake_minimum_required(VERSION 3.16)

project(pathstr LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_TESTS "Enable building of unit tests" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_library(pathstr STATIC
  pathstr.cpp
  pathstr.h
)

target_link_libraries(pathstr PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(pathstr PRIVATE PATHSTR_LIBRARY)

if(WITH_TESTS)
  enable_testing()
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Test)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

  add_executable(test_pathstr test_pathstr.cpp)
  add_test(NAME test_pathstr COMMAND test_pathstr)

  target_link_libraries(test_pathstr PRIVATE Qt${QT_VERSION_MAJOR}::Test pathstr)
endif(WITH_TESTS)
