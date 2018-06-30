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

#include <aws/testing/aws_test_harness.h>

#include <aws/compression/huffman.h>
#include <aws/compression/private/huffman_static_decode.h>

struct code_point {
    uint16_t symbol;
    uint8_t num_bits;
    uint32_t bit_pattern;
};

static struct code_point code_points[] = {
#define HUFFMAN_CODE(psymbol, pbit_string, pbit_pattern, pnum_bits) { .symbol = psymbol, .num_bits = pnum_bits, .bit_pattern = pbit_pattern },
#include <aws/compression/private/h2_huffman_static_table.def>
#undef HUFFMAN_CODE
};

static int test_huffman_character_encoder(struct aws_allocator *allocator, void *user_data) {

    struct aws_huffman_coder *coder = hpack_get_coder();

    /* Validate all characters decode correctly */
    for (int i = 0; i < sizeof(code_points) / sizeof(struct code_point); ++i) {
        struct code_point *value = &code_points[i];

        uint32_t bit_pattern = value->bit_pattern << (32 - value->num_bits);

        uint16_t out;
        size_t bits_read = coder->decode(bit_pattern, &out, NULL);

        ASSERT_UINT_EQUALS(value->symbol, out, "Read incorrect symbol");
        ASSERT_UINT_EQUALS(value->num_bits, bits_read, "Read incorrect number of bits");
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_character_encoder, test_huffman_character_encoder)

static int test_huffman_encoder(struct aws_allocator *allocator, void *user_data) {

    const char input_buffer[] = "www.example.com";
    uint8_t expected[] = { 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff, 0xff, 0xff, 0xff };
    uint8_t output_buffer[sizeof(expected) + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_coder *coder = hpack_get_coder();
    size_t bytes_written = aws_huffman_encode(coder, input_buffer, sizeof(input_buffer) - 1, output_buffer);
    ASSERT_UINT_EQUALS(bytes_written, 15, "Encoder wrote too many bytes");
    ASSERT_UINT_EQUALS(output_buffer[15], 0, "Encoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(expected, 15, output_buffer, bytes_written, "Data written is incorrect!");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder, test_huffman_encoder)

static int test_huffman_encoder_all_code_points(struct aws_allocator *allocator, void *user_data) {

    const char input_buffer[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    uint8_t expected[] = {
        0x53, 0xf8, 0xfe, 0x7f, 0xeb, 0xff, 0x2a, 0xfc, 0x7f, 0xaf, 0xeb, 0xfb, 0xf9, 0xff, 0x7f, 0x4b, 0x2e, 0xc0, 0x02, 0x26, 0x5a,
        0x6d, 0xc7, 0x5e, 0x7e, 0xe7, 0xdf, 0xff, 0xc8, 0x3f, 0xef, 0xfc, 0xff, 0xd4, 0x37, 0x6f, 0x5f, 0xc1, 0x87, 0x16, 0x3c, 0x99,
        0x73, 0x67, 0xd1, 0xa7, 0x56, 0xbd, 0x9b, 0x77, 0x6f, 0xe1, 0xc7, 0x97, 0xe7, 0x3f, 0xdf, 0xfd, 0xff, 0xff, 0x0f, 0xfe, 0x7f,
        0xf9, 0x17, 0xff, 0xd1, 0xc6, 0x49, 0x0b, 0x2c, 0xd3, 0x9b, 0xa7, 0x5a, 0x29, 0xa8, 0xf5, 0xf6, 0xb1, 0x9, 0xb7, 0xbf, 0x8f,
        0x3e, 0xbd, 0xff, 0xfe, 0xff, 0x9f, 0xfe, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xff,
    };
    uint8_t output_buffer[sizeof(expected) + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_coder *coder = hpack_get_coder();
    size_t bytes_written = aws_huffman_encode(coder, input_buffer, sizeof(input_buffer) - 1, output_buffer);
    ASSERT_UINT_EQUALS(bytes_written, sizeof(expected), "Encoder wrote too many bytes");
    ASSERT_UINT_EQUALS(output_buffer[sizeof(expected)], 0, "Encoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(expected, sizeof(expected), output_buffer, bytes_written, "Data written is incorrect!");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder_all_code_points, test_huffman_encoder_all_code_points)

static int test_huffman_character_decoder(struct aws_allocator *allocator, void *user_data) {

    struct aws_huffman_coder *coder = hpack_get_coder();

    /* Validate all characters decode correctly */
    for (int i = 0; i < sizeof(code_points) / sizeof(struct code_point); ++i) {
        struct code_point *value = &code_points[i];

        uint32_t bit_pattern = value->bit_pattern << (32 - value->num_bits);

        uint16_t out;
        size_t bits_read = coder->decode(bit_pattern, &out, NULL);

        ASSERT_UINT_EQUALS(value->symbol, out, "Read incorrect symbol");
        ASSERT_UINT_EQUALS(value->num_bits, bits_read, "Read incorrect number of bits");
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_character_decoder, test_huffman_character_decoder)

static int test_huffman_decoder(struct aws_allocator *allocator, void *user_data) {

    uint8_t input_buffer[] = { 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff, 0xff, 0xff, 0xff };
    const char expected[] = "www.example.com";
    char output_buffer[sizeof(expected) + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_coder *coder = hpack_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = 0;
    aws_huffman_decoder_state state = aws_huffman_decode(&decoder, input_buffer, sizeof(input_buffer), output_buffer, &output_size, &processed);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(sizeof(expected) - 1, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(sizeof(input_buffer), processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_buffer[sizeof(expected)], 0, "Decoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(expected, sizeof(expected) - 1, output_buffer, output_size, "Data written is incorrect");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder, test_huffman_decoder)

static int test_huffman_decoder_all_code_points(struct aws_allocator *allocator, void *user_data) {

    uint8_t input_buffer[] = {
        0x53, 0xf8, 0xfe, 0x7f, 0xeb, 0xff, 0x2a, 0xfc, 0x7f, 0xaf, 0xeb, 0xfb, 0xf9, 0xff, 0x7f, 0x4b, 0x2e, 0xc0, 0x02, 0x26, 0x5a,
        0x6d, 0xc7, 0x5e, 0x7e, 0xe7, 0xdf, 0xff, 0xc8, 0x3f, 0xef, 0xfc, 0xff, 0xd4, 0x37, 0x6f, 0x5f, 0xc1, 0x87, 0x16, 0x3c, 0x99,
        0x73, 0x67, 0xd1, 0xa7, 0x56, 0xbd, 0x9b, 0x77, 0x6f, 0xe1, 0xc7, 0x97, 0xe7, 0x3f, 0xdf, 0xfd, 0xff, 0xff, 0x0f, 0xfe, 0x7f,
        0xf9, 0x17, 0xff, 0xd1, 0xc6, 0x49, 0x0b, 0x2c, 0xd3, 0x9b, 0xa7, 0x5a, 0x29, 0xa8, 0xf5, 0xf6, 0xb1, 0x9, 0xb7, 0xbf, 0x8f,
        0x3e, 0xbd, 0xff, 0xfe, 0xff, 0x9f, 0xfe, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xff,
    };
    const char expected[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    char output_buffer[sizeof(expected) + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_coder *coder = hpack_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = 0;
    aws_huffman_decoder_state state = aws_huffman_decode(&decoder, input_buffer, sizeof(input_buffer), output_buffer, &output_size, &processed);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(sizeof(expected) - 1, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(sizeof(input_buffer), processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_buffer[sizeof(expected)], 0, "Decoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(expected, sizeof(expected) - 1, output_buffer, output_size, "Data written is incorrect");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_all_code_points, test_huffman_decoder_all_code_points)

static int test_huffman_transitive(struct aws_allocator *allocator, void *user_data) {

    struct aws_huffman_coder *coder = hpack_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    const char input_string[] = "www.example.com";
    uint8_t intermediate_buffer[16];
    AWS_ZERO_ARRAY(intermediate_buffer);
    char output_string[16];
    AWS_ZERO_ARRAY(output_string);

    size_t bytes_written = aws_huffman_encode(coder, input_string, sizeof(input_string) - 1, intermediate_buffer);

    ASSERT_UINT_EQUALS(bytes_written, 15, "Encoder wrote too many bytes");
    ASSERT_UINT_EQUALS(intermediate_buffer[15], 0, "Encoder wrote too far!");

    size_t output_size = sizeof(output_string);
    size_t processed = 0;
    aws_huffman_decoder_state state = aws_huffman_decode(&decoder, intermediate_buffer, bytes_written, output_string, &output_size, &processed);

    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(sizeof(input_string) - 1, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(bytes_written, processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_string[15], 0, "Decoder wrote too far!");

    ASSERT_BIN_ARRAYS_EQUALS(input_string, sizeof(input_string) - 1, output_string, output_size, "Strings at begin and end don't match");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_transitive, test_huffman_transitive)

static int test_huffman_transitive_all_code_points(struct aws_allocator *allocator, void *user_data) {

    struct aws_huffman_coder *coder = hpack_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    const char input_string[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    uint8_t intermediate_buffer[98];
    AWS_ZERO_ARRAY(intermediate_buffer);
    char output_string[sizeof(input_string) + 1];
    AWS_ZERO_ARRAY(output_string);

    size_t bytes_written = aws_huffman_encode(coder, input_string, sizeof(input_string) - 1, intermediate_buffer);

    ASSERT_UINT_EQUALS(bytes_written, 97, "Encoder wrote too many bytes");
    ASSERT_UINT_EQUALS(intermediate_buffer[97], 0, "Encoder wrote too far!");

    size_t output_size = sizeof(output_string);
    size_t processed = 0;
    aws_huffman_decoder_state state = aws_huffman_decode(&decoder, intermediate_buffer, bytes_written, output_string, &output_size, &processed);

    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(sizeof(input_string) - 1, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(bytes_written, processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_string[sizeof(input_string)], 0, "Decoder wrote too far!");

    ASSERT_BIN_ARRAYS_EQUALS(input_string, sizeof(input_string) - 1, output_string, output_size, "Strings at begin and end don't match");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_transitive_all_code_points, test_huffman_transitive_all_code_points)
