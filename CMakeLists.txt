cmake_minimum_required(VERSION 3.1)
project(aws-c-compression C)

if (DEFINED CMAKE_PREFIX_PATH)
    file(TO_CMAKE_PATH ${CMAKE_PREFIX_PATH} CMAKE_PREFIX_PATH)
endif()

if (DEFINED CMAKE_INSTALL_PREFIX)
    file(TO_CMAKE_PATH ${CMAKE_INSTALL_PREFIX} CMAKE_INSTALL_PREFIX)
endif()

if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
elseif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_PREFIX_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake")

include(AwsCFlags)
include(AwsSanitizers)
include(CheckCCompilerFlag)

file(GLOB AWS_COMPRESSION_HEADERS
        "include/aws/compression/*.h"
        )

file(GLOB AWS_COMPRESSION_PRIV_HEADERS
        "include/aws/compression/private/*.h"
        )

file(GLOB AWS_COMPRESSION_TESTING_HEADERS
        "include/aws/testing/compression/*.h"
        "include/aws/testing/compression/*.inl"
        )

file(GLOB AWS_COMPRESSION_SRC
        "source/*.c"
        )

file(GLOB COMPRESSION_HEADERS
        ${AWS_COMPRESSION_HEADERS}
        ${AWS_COMPRESSION_PRIV_HEADERS}
        ${AWS_COMPRESSION_TESTING_HEADERS}
        )

file(GLOB COMPRESSION_SRC
        ${AWS_COMPRESSION_SRC}
        )

add_library(${CMAKE_PROJECT_NAME} ${LIBTYPE} ${COMPRESSION_HEADERS} ${COMPRESSION_SRC})
aws_set_common_properties(${CMAKE_PROJECT_NAME})
aws_add_sanitizers(${CMAKE_PROJECT_NAME})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

find_package(aws-c-common REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC AWS::aws-c-common)

install(FILES ${AWS_COMPRESSION_HEADERS} DESTINATION "include/aws/compression")
install(FILES ${AWS_COMPRESSION_TESTING_HEADERS} DESTINATION "include/aws/testing/compression")

install(
        TARGETS ${CMAKE_PROJECT_NAME} EXPORT ${CMAKE_PROJECT_NAME}-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        COMPONENT library
)

export(TARGETS ${CMAKE_PROJECT_NAME} FILE ${CMAKE_PROJECT_NAME}-config.cmake)
install(EXPORT ${CMAKE_PROJECT_NAME}-config DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_PROJECT_NAME}/cmake/")

add_subdirectory(source/huffman_generator)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
