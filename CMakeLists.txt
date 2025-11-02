cmake_minimum_required(VERSION 3.16)
project(dull)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt6_wrap_ui(UI_HEADERS src/mainwindow.ui)

add_executable(${PROJECT_NAME} src/main.cc ${UI_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)
