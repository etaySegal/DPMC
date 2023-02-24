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
find_library(COLAMD NAMES colamd PATHS /usr/local/lib/)
#Relevant for all libs
include_directories(/usr/local/include/)

# EXE
set(EXE_TARGET htb)
set(EXE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}../../common/src/common.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/htb.cc)
add_executable(${EXE_TARGET} "")
target_compile_options(${EXE_TARGET} PRIVATE ${COMMON_FLAGS})
target_include_directories(${EXE_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${EXE_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${EXE_TARGET} PRIVATE ${GMP} ${GMPXX} ${COLAMD})
target_sources(${EXE_TARGET} PRIVATE ${EXE_SOURCES})

#LIB
#set(LIB_TARGET htb_lib)
#set(LIB_SOURCES src/bone_dl.cpp)
#add_library(${LIB_TARGET} STATIC "")
#target_sources(${LIB_TARGET} PRIVATE ${LIB_SOURCES})
#target_include_directories(${LIB_TARGET} PRIVATE include)

#set(INSTALL_DIR ${PROJECT_NAME}-${PROJECT_VERSION})

#set_target_properties(${LIB_TARGET} PROPERTIES VERSION ${PROJECT_VERSION})
#set_target_properties(${LIB_TARGET} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
#target_include_directories(${LIB_TARGET}
#        PRIVATE
#        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
#        PUBLIC
#        /usr/local/ge/opt/ig_libraries/glm/include
#        $<INSTALL_INTERFACE:${INSTALL_DIR}/include>
#        )
