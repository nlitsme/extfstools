cmake_minimum_required(VERSION 3.10)
project(extfstools)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmfind")

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

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
# NOTE: I specified date_time here, while in fact i need interprocess.
# Problem is that 'interprocess' is not recognized by the FindBoost.cmake script,
# so I substituted an arbitrary lib which is there.
find_package(Boost COMPONENTS date_time)
if (Boost_FOUND)
    add_library(BoostHeaders INTERFACE)
    target_include_directories(BoostHeaders INTERFACE ${Boost_INCLUDE_DIRS})
    target_compile_definitions(BoostHeaders INTERFACE BOOST_ALL_NO_LIB)
    message(NOTICE "Found boost at ${Boost_INCLUDE_DIRS}")

    set(MMAPLIB BoostHeaders)
else()
    if (WIN32)
        include(FetchContent)
        FetchContent_Populate(boost
            URL      https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2
        )
        # todo: check fetch result.
        add_library(BoostHeaders INTERFACE)
        target_include_directories(BoostHeaders INTERFACE ${CMAKE_BINARY_DIR}/boost-src)
        target_compile_definitions(BoostHeaders INTERFACE BOOST_ALL_NO_LIB)
        set(MMAPLIB BoostHeaders)
    else()
        find_package(cpputils)
        if (cpputils_FOUND)
            message(NOTICE "uisng cpputils")
            set(MMAPLIB cpputils)
            add_definitions(-DUSE_CPPUTILS)
        endif()
    endif()
endif()
if (NOT MMAPLIB)
    message(FATAL_ERROR "We need either boost::interprocess, or cpputils/mmfile.h")
endif()

add_executable(ext2rd ext2rd.cpp)
target_link_libraries(ext2rd  itslib)
target_link_libraries(ext2rd ${MMAPLIB})


