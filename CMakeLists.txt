cmake_minimum_required(VERSION 3.10)
project(extfstools)
set(CMAKE_CXX_STANDARD 11)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(DARWIN TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(FREEBSD TRUE)
endif()


# checking for if we are called in the correct way:
#  with a -B argument.  and without a cache file in the source directory.
if (CMAKE_CACHEFILE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
    message(FATAL_ERROR "\nUnexpected CMakeCache.txt file in the source directory. Please remove it.")
    return()
endif()

if (EXISTS ${CMAKE_BINARY_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "\nRun cmake with an explicit -B buildpath")
    return()
endif()

if(MSVC)
    # /MP = multithreaded build
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    # /utf-8 = utf8 source and execution
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

if (WIN32)
    add_definitions(-DNOMINMAX -DNOGDI)
endif()


add_library(itslib STATIC itslib/src/stringutils.cpp)
target_include_directories(itslib PUBLIC itslib)
target_compile_definitions(itslib PUBLIC _UNIX _NO_RAPI)

#add_library(cpputils INTERFACE)
#target_include_directories(cpputils INTERFACE ../cpputils)

find_package(Boost COMPONENTS headers)

add_executable(ext2rd ext2rd.cpp)
target_link_libraries(ext2rd Boost::headers itslib)

add_executable(ext2dump ext2dump.cpp)
target_link_libraries(ext2dump Boost::headers itslib)


