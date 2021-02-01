file(GLOB CPPUTILS_DIRS cpputils  submodules/cpputils ../cpputils ../../cpputils)
find_path(CPPUTILS_PATH NAMES string-lineenum.h PATHS ${CPPUTILS_DIRS})
if(NOT CPPUTILS_PATH)
    include(FetchContent)
    FetchContent_Populate(cpputils
        GIT_REPOSITORY https://github.com/nlitsme/cpputils)
    # TODO: check fetch result

    set(CPPUTILS_PATH ${CMAKE_BINARY_DIR}/cpputils-src)
endif()

if(NOT CPPUTILS_PATH)
    return()
endif()
add_library(cpputils INTERFACE)
target_include_directories(cpputils INTERFACE ${CPPUTILS_PATH})

set(cpputils_FOUND TRUE)
