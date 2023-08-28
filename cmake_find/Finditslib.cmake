if (TARGET itslib)
    return()
endif()
find_path(ITSLIB_PATH NAMES include/itslib/stringutils.h PATHS ${CMAKE_SOURCE_DIR}/symlinks/itslib)
if(ITSLIB_PATH STREQUAL "ITSLIB_PATH-NOTFOUND")
    include(FetchContent)
    FetchContent_Populate(itslib
        GIT_REPOSITORY https://github.com/nlitsme/legacy-itslib-library)
    set(ITSLIB_PATH  ${itslib_SOURCE_DIR})
else()
    set(itslib_BINARY_DIR ${CMAKE_BINARY_DIR}/itslib-build)
endif()

add_subdirectory(${ITSLIB_PATH})

