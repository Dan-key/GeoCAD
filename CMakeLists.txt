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
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

message(STATUS ${SOURCES})

set(QRC_NAME ${PROJECT_NAME}.qrc)
set(QRC_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${QRC_NAME})

# Build
GenerateQrc(${QRC_NAME} "/" ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} ${SOURCES} ${QRC_FILENAME})

file(GLOB SPVASM_FILES "${CMAKE_SOURCE_DIR}/src/UI/shaders/*")
if(NOT SPVASM_FILES)
    message(WARNING "No .spvasm files found in ${CMAKE_SOURCE_DIR}/src/UI/shaders/")
endif()

# Map inputs to outputs in build directory (e.g., ${CMAKE_BINARY_DIR}/shaders)
set(SPV_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPV_OUT_DIR})

set(SPV_OUT_FILES "")
foreach(infile IN LISTS SPVASM_FILES)
    get_filename_component(fname ${infile} NAME_WE)   # base name without ext
    set(outfile ${SPV_OUT_DIR}/${fname}.spv)
    list(APPEND SPV_OUT_FILES ${outfile})

    add_custom_command(
        OUTPUT ${outfile}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SPV_OUT_DIR}
        COMMAND glslangValidator -V ${infile} -o ${outfile}
        DEPENDS ${infile}
        COMMENT "Assembling ${infile} -> ${outfile}"
        VERBATIM
    )
endforeach()
add_custom_target(spirv_shaders ALL DEPENDS ${SPV_OUT_FILES})

# Ensure the executable depends on the shaders being built
add_dependencies(${PROJECT_NAME} spirv_shaders)

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Qml Qt6::Quick Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)