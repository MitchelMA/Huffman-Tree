file(GLOB_RECURSE PRIVATE_SOURCES ./private/*.c)

add_executable(HuffmanTree
    main.c
    ${PRIVATE_SOURCES})

target_compile_options(HuffmanTree
    PUBLIC
    ${BUILD_PARAMS})

target_include_directories(HuffmanTree
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include)
