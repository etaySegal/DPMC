cmake_minimum_required(VERSION 3.10)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
project(htb)

macro(remove_cxx_flag flag)
    string(REPLACE "${flag}" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endmacro()
# debug only flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -O0" )
# release only flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
remove_cxx_flag("-DNDEBUG")

set(COMMON_FLAGS -Wno-multichar -std=c++20)
find_library(GMP NAMES gmp PATHS /usr/local/lib/)
find_library(GMPXX NAMES gmpxx PATHS /usr/local/lib/)
#Relevant for all libs
include_directories(/usr/local/include/)

add_executable(${PROJECT_NAME} "")
target_link_libraries(${PROJECT_NAME} PRIVATE ${GMP} ${GMPXX})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_FLAGS})

########################################################################################################################
# FILES TO COMPILE
########################################################################################################################
SET(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}../../common/src/common.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/htb.cc
        )

# add the c/cc files to be compiled by the targets
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})