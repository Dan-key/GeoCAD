cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME GeoCAD)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

file(GLOB SOURCES "*.cpp")

set(RESOURCE_FILES resources.qrc)

add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCE_FILES})

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)
qt6_add_resources(${PROJECT_NAME} "resources" PREFIX "/"
    FILES ${RESOURCE_FILES}
)
