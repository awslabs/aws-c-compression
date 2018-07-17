include(CTest)
enable_testing()

file(GLOB TEST_SRC "main.c" "test_huffman_static.c")
file(GLOB TEST_HDRS "*.h")
file(GLOB TESTS ${TEST_HDRS} ${TEST_SRC})

set(TEST_BINARY_NAME ${CMAKE_PROJECT_NAME}-tests)

add_executable(${TEST_BINARY_NAME} ${TESTS})
target_link_libraries(${TEST_BINARY_NAME} ${CMAKE_PROJECT_NAME})
aws_set_common_properties(${TEST_BINARY_NAME})
aws_add_sanitizers(${TEST_BINARY_NAME} ${${CMAKE_PROJECT_NAME}_SANITIZERS})
target_compile_definitions(${TEST_BINARY_NAME} PRIVATE AWS_UNSTABLE_TESTING_API=1)
target_include_directories(${TEST_BINARY_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
if (MSVC)
    target_compile_definitions(${TEST_BINARY_NAME} PRIVATE "-D_CRT_SECURE_NO_WARNINGS")
endif ()

add_test(huffman_symbol_encoder ${TEST_BINARY_NAME} huffman_symbol_encoder)
add_test(huffman_encoder ${TEST_BINARY_NAME} huffman_encoder)
add_test(huffman_encoder_all_code_points ${TEST_BINARY_NAME} huffman_encoder_all_code_points)
add_test(huffman_encoder_partial_output ${TEST_BINARY_NAME} huffman_encoder_partial_output)

add_test(huffman_symbol_decoder ${TEST_BINARY_NAME} huffman_symbol_decoder)
add_test(huffman_decoder ${TEST_BINARY_NAME} huffman_decoder)
add_test(huffman_decoder_all_code_points ${TEST_BINARY_NAME} huffman_decoder_all_code_points)
add_test(huffman_decoder_partial_input ${TEST_BINARY_NAME} huffman_decoder_partial_input)
add_test(huffman_decoder_partial_output ${TEST_BINARY_NAME} huffman_decoder_partial_output)

add_test(huffman_transitive ${TEST_BINARY_NAME} huffman_transitive)
add_test(huffman_transitive_all_code_points ${TEST_BINARY_NAME} huffman_transitive_all_code_points)
add_test(huffman_transitive_chunked ${TEST_BINARY_NAME} huffman_transitive_chunked)

option(ENABLE_FUZZ_TESTS "Build and run fuzz tests" OFF)
set(FUZZ_TESTS_MAX_TIME 60 CACHE STRING "Max time to run each fuzz test")

if(ENABLE_FUZZ_TESTS)
    aws_check_sanitizer(fuzzer)
    if (NOT HAS_SANITIZER_fuzzer)
        message(FATAL_ERROR "ENABLE_FUZZ_TESTS is set but the current compiler (${CMAKE_CXX_COMPILER_ID}) doesn't support -fsanitize=fuzzer")
    endif()

    file(GLOB fuzz_files "fuzz/*.c")
    foreach(test_file ${fuzz_files})
        get_filename_component(TEST_FILE_NAME ${test_file} NAME_WE)

        set(FUZZ_BINARY_NAME ${CMAKE_PROJECT_NAME}-fuzz-${TEST_FILE_NAME})
        add_executable(${FUZZ_BINARY_NAME} ${test_file} "test_huffman_static.c")
        target_link_libraries(${FUZZ_BINARY_NAME} ${CMAKE_PROJECT_NAME})
        aws_set_common_properties(${FUZZ_BINARY_NAME})
        aws_add_sanitizers(${FUZZ_BINARY_NAME} SANITIZERS "${${CMAKE_PROJECT_NAME}_SANITIZERS};fuzzer")
        target_compile_definitions(${FUZZ_BINARY_NAME} PRIVATE AWS_UNSTABLE_TESTING_API=1)
        target_include_directories(${FUZZ_BINARY_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

        add_test(NAME huffman_fuzz_${TEST_FILE_NAME} COMMAND ${FUZZ_BINARY_NAME} -timeout=1 -max_total_time=${FUZZ_TESTS_MAX_TIME})
    endforeach()
endif()
