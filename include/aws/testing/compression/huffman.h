#ifndef AWS_COMPRESSION_TESTING_HUFFMAN_H
#define AWS_COMPRESSION_TESTING_HUFFMAN_H

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

#include <aws/compression/huffman.h>

#include <stdint.h>

/**
 * The intended use of file is to allow testing of huffman character coders.
 * By doing the following, you can ensure the output of encoders decoders are correct:
 * \code{c}
 * static struct huffman_test_code_point code_points[] = {
 * #include "test_huffman_static_table.def"
 * };
 * \endcode
 * You may then iterate over each code point in the array, and test the following (pseudo-code):
 * \code{c}
 * for (cp in code_points) {
 *     assert(my_coder->encode(cp.symbol) == cp.pattern);
 *     assert(my_coder->decode(cp.pattern) == cp.symbol);
 * }
 * \endcode
 */

/**
 * Structure containing all relevant information about a code point
 */
struct huffman_test_code_point {
    uint16_t symbol;
    struct aws_huffman_code code;
};

/**
 * Macro to be used when including a table def file, populates an array of huffman_test_code_points
 */
#define HUFFMAN_CODE(psymbol, pbit_string, pbit_pattern, pnum_bits) { .symbol = psymbol, .code = { .pattern = pbit_pattern, .num_bits = pnum_bits } },

#endif /* AWS_COMPRESSION_TESTING_HUFFMAN_H */
