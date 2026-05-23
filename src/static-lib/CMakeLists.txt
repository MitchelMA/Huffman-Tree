file(GLOB PRIVATE_SOURCES ./private/*.c)

add_library(${PROJECT_NAME}-static
    STATIC
    ${PRIVATE_SOURCES})

target_link_libraries(${PROJECT_NAME}-static
    PUBLIC
    CVector)

target_compile_options(${PROJECT_NAME}-static
    PRIVATE
    ${BUILD_PARAMS})

target_include_directories(${PROJECT_NAME}-static
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include)
