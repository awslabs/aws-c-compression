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

struct huffman_test_code_point {
    uint16_t symbol;
    struct aws_huffman_bit_pattern pattern;
};

#define HUFFMAN_CODE(psymbol, pbit_string, pbit_pattern, pnum_bits) { .symbol = psymbol, .pattern = { .pattern = pbit_pattern, .num_bits = pnum_bits } },

#endif /* AWS_COMPRESSION_TESTING_HUFFMAN_H */
