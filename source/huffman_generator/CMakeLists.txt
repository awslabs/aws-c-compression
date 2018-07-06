file(GLOB GENERATIR_SRC "generator.c")

set(GENERATOR_BINARY_NAME ${CMAKE_PROJECT_NAME}-huffman-generator)

add_executable(${GENERATOR_BINARY_NAME} ${GENERATIR_SRC})
set_target_properties(${GENERATOR_BINARY_NAME} PROPERTIES LINKER_LANGUAGE C C_STANDARD 99)
target_include_directories(${GENERATOR_BINARY_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )

if (MSVC)
    target_compile_options(${GENERATOR_BINARY_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${GENERATOR_BINARY_NAME} PRIVATE -Wall -Wno-long-long -Werror)
endif ()

install(
        TARGETS ${GENERATOR_BINARY_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
