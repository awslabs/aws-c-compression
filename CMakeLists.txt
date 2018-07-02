cmake_minimum_required(VERSION 3.1)
project(aws-c-compression)

option(BUILD_PORTABLE_BINARIES
        "Build Relocatable Binaries, this will turn off features that will fail on older kernels than used for the build."
        OFF)

file(GLOB AWS_COMPRESSION_HEADERS
        "include/aws/compression/*.h"
        )

file(GLOB AWS_COMPRESSION_PRIV_HEADERS
        "include/aws/compression/private/*.h"
        )

file(GLOB AWS_COMPRESSION_SRC
        "source/*.c"
        )

file(GLOB COMPRESSION_HEADERS
        ${AWS_COMPRESSION_HEADERS}
        ${AWS_COMPRESSION_PRIV_HEADERS}
        )

file(GLOB COMPRESSION_SRC
        ${AWS_COMPRESSION_SRC}
        )

add_library(${CMAKE_PROJECT_NAME} ${LIBTYPE} ${COMPRESSION_HEADERS} ${COMPRESSION_SRC})
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

set(CMAKE_C_FLAGS_DEBUGOPT "")
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY C_STANDARD 99)

#set warnings
if (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -pedantic -Werror -Wno-long-long)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wgnu)
    endif()
endif ()

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

find_package(aws-c-common REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} aws-c-common ${PLATFORM_LIBS})

install(FILES ${AWS_COMPRESSION_HEADERS} DESTINATION "include/aws/compression")

install(
        TARGETS ${CMAKE_PROJECT_NAME} EXPORT ${CMAKE_PROJECT_NAME}-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        COMPONENT library
)

export(TARGETS ${CMAKE_PROJECT_NAME} FILE ${CMAKE_PROJECT_NAME}-config.cmake)
install(EXPORT ${CMAKE_PROJECT_NAME}-config DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_PROJECT_NAME}/cmake/")

include(CTest)
add_subdirectory(tests)
add_subdirectory(source/huffman_generator)
