cmake_minimum_required(VERSION 3.16)

project("iCE && tEA File Service" VERSION 1.0 LANGUAGES CXX)

# OS checking
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This project can only be built on Linux")
endif()

# Architecture checking (64-bit only)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This project can only be built on 64-bit Linux architectures")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/release)
set(RUNTIME_OUTPUT_NAME filesvc)

set(INC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/roles/common
    ${CMAKE_CURRENT_SOURCE_DIR}/roles/client
    ${CMAKE_CURRENT_SOURCE_DIR}/roles/server
)

set(SRC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/roles/common
    ${CMAKE_CURRENT_SOURCE_DIR}/roles/client
    ${CMAKE_CURRENT_SOURCE_DIR}/roles/server
)

set(HEADERS "")
foreach(dir ${INC_DIRS})
    file(GLOB tmp ${dir}/*.h)
    list(APPEND HEADERS ${tmp})
endforeach()

set(SOURCES "")
foreach(dir ${SRC_DIRS})
    file(GLOB tmp ${dir}/*.cpp)
    list(APPEND SOURCES ${tmp})
endforeach()

# openSSL
find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS ON)
pkg_check_modules(OPENSSL REQUIRED openssl)

add_executable(${RUNTIME_OUTPUT_NAME}
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${RUNTIME_OUTPUT_NAME}
    PRIVATE
        ${OPENSSL_INCLUDE_DIRS}
        ${INC_DIRS}
)

target_link_libraries(${RUNTIME_OUTPUT_NAME}
    PRIVATE
        ${OPENSSL_LIBRARIES}
)
