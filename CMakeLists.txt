cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME GeoCAD)
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake")

include("GenerateQrc")

find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

file(GLOB SOURCES "*.cpp")

set(QRC_NAME ${PROJECT_NAME}.qrc)
set(QRC_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${QRC_NAME})

# Build
GenerateQrc(${QRC_NAME} "/" ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} ${SOURCES} ${QRC_FILENAME})

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Qml Qt6::Quick)