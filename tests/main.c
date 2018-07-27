/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#if _MSC_VER
#    pragma warning(disable : 4100) /* unreferenced formal parameter */
#    pragma warning(disable : 4204) /* non-constant aggregate initializer */
#    pragma warning(disable : 4221) /* initialization using address of         \
                                       automatic variable */
#endif

#include <aws/testing/aws_test_harness.h>

#include "huffman_test.c"

static int run_tests(int argc, char *argv[]) {
    AWS_RUN_TEST_CASES(
        &huffman_symbol_encoder,
        &huffman_encoder,
        &huffman_encoder_all_code_points,
        &huffman_encoder_partial_output,

        &huffman_symbol_decoder,
        &huffman_decoder,
        &huffman_decoder_all_code_points,
        &huffman_decoder_partial_input,
        &huffman_decoder_partial_output,

        &huffman_transitive,
        &huffman_transitive_all_code_points,
        &huffman_transitive_chunked);
}

int main(int argc, char *argv[]) {
    int ret_val = run_tests(argc, argv);
    return ret_val;
}
