include(CTest)
enable_testing()

file(GLOB TEST_SRC "main.c")
file(GLOB TEST_HDRS "*.h")
file(GLOB TESTS ${TEST_HDRS} ${TEST_SRC})

set(TEST_BINARY_NAME ${CMAKE_PROJECT_NAME}-tests)

add_executable(${TEST_BINARY_NAME} ${TESTS})
target_link_libraries(${TEST_BINARY_NAME} ${CMAKE_PROJECT_NAME})
set_target_properties(${TEST_BINARY_NAME} PROPERTIES LINKER_LANGUAGE C C_STANDARD 99)
target_compile_definitions(${TEST_BINARY_NAME} PRIVATE AWS_UNSTABLE_TESTING_API=1)
target_include_directories(${TEST_BINARY_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
if (MSVC)
    target_compile_options(${TEST_BINARY_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${TEST_BINARY_NAME} PRIVATE -Wall -Wno-long-long -Werror)
endif ()

add_test(huffman_character_encoder ${TEST_BINARY_NAME} huffman_character_encoder)
add_test(huffman_encoder ${TEST_BINARY_NAME} huffman_encoder)
add_test(huffman_encoder_all_code_points ${TEST_BINARY_NAME} huffman_encoder_all_code_points)
add_test(huffman_encoder_partial_output ${TEST_BINARY_NAME} huffman_encoder_partial_output)

add_test(huffman_character_decoder ${TEST_BINARY_NAME} huffman_character_decoder)
add_test(huffman_decoder ${TEST_BINARY_NAME} huffman_decoder)
add_test(huffman_decoder_all_code_points ${TEST_BINARY_NAME} huffman_decoder_all_code_points)
add_test(huffman_decoder_partial_input ${TEST_BINARY_NAME} huffman_decoder_partial_input)
add_test(huffman_decoder_partial_output ${TEST_BINARY_NAME} huffman_decoder_partial_output)

add_test(huffman_transitive ${TEST_BINARY_NAME} huffman_transitive)
add_test(huffman_transitive_all_code_points ${TEST_BINARY_NAME} huffman_transitive_all_code_points)
