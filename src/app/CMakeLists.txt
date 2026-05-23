add_executable(${PROJECT_NAME}
    main.c)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${PROJECT_NAME}-static)

    #$<$<STREQUAL:$<UPPER_CASE:${CMAKE_BUILD_TYPE}>,DEBUG>:asan ubsan>)

target_compile_options(${PROJECT_NAME}
    PUBLIC
    ${BUILD_PARAMS})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include)
