cmake_minimum_required(VERSION 3.25)
include (FetchContent)

project(HuffmanTree
    VERSION 0.0.1
    DESCRIPTION "A mini project to test out things with a huffman tree and variable-length encodings"
    LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

# General compile options
if (NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -pedantic -Wundef")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb -O0")
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(BUILD_PARAMS
        /std:c17
        /W3
        /wd4996 # <- suppresses warning C4996: 'strdup' is a deprecated POSIX name
        $<$<STREQUAL:$<UPPER_CASE:${CMAKE_BUILD_TYPE}>,DEBUG>:/Od>
        $<$<STREQUAL:$<UPPER_CASE:${CMAKE_BUILD_TYPE}>,RELEASE>:/WX /Oi /Ox>)
else()
    set(BUILD_PARAMS
        -std=c23
        -Wall -Wextra -Wpedantic -pedantic -Wundef
        $<$<STREQUAL:$<UPPER_CASE:${CMAKE_BUILD_TYPE}>,DEBUG>:-O0 -g -ggdb -fsanitize=address,undefined,leak>
        $<$<STREQUAL:$<UPPER_CASE:${CMAKE_BUILD_TYPE}>,RELEASE>:-Werror -O3>)
endif()

FetchContent_Declare(
    CVector
    GIT_REPOSITORY https://github.com/MitchelMA/CVector.git
    GIT_TAG 1.0.0
)
FetchContent_Declare(
    CString
    GIT_REPOSITORY https://github.com/MitchelMA/cstring.git
    GIT_TAG main
)
FetchContent_Declare(
    CCommandArgParser
    GIT_REPOSITORY https://github.com/MitchelMA/CCommandArgParser.git
    GIT_TAG main
)

FetchContent_MakeAvailable(CVector)

add_subdirectory(src)
