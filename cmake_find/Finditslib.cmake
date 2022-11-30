find_path(ITSLIB_PATH NAMES include/itslib/stringutils.h PATHS symlinks/itslib)
if(ITSLIB_PATH STREQUAL "ITSLIB_PATH-NOTFOUND")
    include(FetchContent)
    FetchContent_Populate(itslib
        GIT_REPOSITORY https://github.com/nlitsme/legacy-itslib-library)
    set(ITSLIB_PATH  ${itslib_SOURCE_DIR})
else()
    set(itslib_BINARY_DIR ${CMAKE_BINARY_DIR}/itslib-build)
endif()
set(ITSLIB_INCLUDE_DIR ${ITSLIB_PATH}/include/itslib)

list(APPEND ITSLIBSRC debug.cpp stringutils.cpp utfcvutils.cpp vectorutils.cpp FileFunctions.cpp)
list(TRANSFORM ITSLIBSRC PREPEND ${ITSLIB_PATH}/src/)
add_library(itslib STATIC ${ITSLIBSRC})
target_include_directories(itslib PUBLIC ${ITSLIB_INCLUDE_DIR})
target_compile_definitions(itslib PUBLIC _NO_RAPI _NO_WINDOWS)
if(NOT WIN32)
    target_compile_definitions(itslib PUBLIC _UNIX)
endif()


