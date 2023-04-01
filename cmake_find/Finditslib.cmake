if (TARGET itslib)
    return()
endif()
find_path(ITSLIB_PATH NAMES include/itslib/stringutils.h PATHS symlinks/itslib)
if(ITSLIB_PATH STREQUAL "ITSLIB_PATH-NOTFOUND")
    include(FetchContent)
    FetchContent_Declare(itslib
        GIT_REPOSITORY https://github.com/nlitsme/legacy-itslib-library
    )
    set(itslib_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/itslib-subbuild)
    FetchContent_MakeAvailable(itslib)    
    set(ITSLIB_PATH  ${itslib_SOURCE_DIR})
else()
    set(itslib_BINARY_DIR ${CMAKE_BINARY_DIR}/itslib-build)
endif()
