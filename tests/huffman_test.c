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
#include <aws/testing/compression/huffman.h>

#include <aws/compression/huffman.h>

#include <stdint.h>

/* Exported by generated file */
struct aws_huffman_character_coder *test_get_coder();

static struct huffman_test_code_point code_points[] = {
#include "test_huffman_static_table.def"
};

/* Useful data for testing */
static const char url_string[] = "www.example.com";
enum { url_string_len = sizeof(url_string) - 1 };
static uint8_t encoded_url[] = { 0x9e, 0x79, 0xeb, 0x9b, 0x04, 0xb3, 0x5a, 0x94, 0xd5, 0xe0, 0x4c, 0xdf, 0xff, };

enum { encoded_url_len = sizeof(encoded_url) };

static const char all_codes[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
enum { all_codes_len = sizeof(all_codes) - 1 };
static uint8_t encoded_codes[] = {
    0x26, 0x9b, 0xa7, 0x69, 0xfa, 0x86, 0xa3, 0xa9, 0x56, 0xd4, 0xf5, 0x4d,
    0x57, 0x56, 0xb9, 0xc4, 0x57, 0xd5, 0xf5, 0x8d, 0x67, 0x5a, 0xd6, 0xf5,
    0xcd, 0x77, 0x5e, 0xd7, 0xf6, 0x0d, 0x87, 0x62, 0xd8, 0xf6, 0x4d, 0x97,
    0x66, 0xba, 0xd9, 0xf6, 0x8b, 0xbc, 0x4e, 0x2b, 0x17, 0x8c, 0xc6, 0xe3,
    0xaf, 0x36, 0x9d, 0xab, 0x1f, 0x90, 0xda, 0xf6, 0xcc, 0x8e, 0xdb, 0xb7,
    0x6d, 0xf7, 0xbb, 0x86, 0x4a, 0xfb, 0x71, 0xc9, 0xee, 0x5b, 0x9e, 0xe9,
    0xba, 0xee, 0xdb, 0xbe, 0xf0, 0x5b, 0x10, 0x42, 0x68, 0xac, 0xc6, 0x7b,
    0xf9, 0x25, 0x99, 0x09, 0xb5, 0x94, 0x52, 0xd8, 0xdc, 0x09, 0xf0, 0x68,
    0xde, 0x77, 0xad, 0xef, 0x7c, 0xdf, 0x7f, 0xff
};
enum { encoded_codes_len = sizeof(encoded_codes) };

static int test_huffman_character_encoder(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding each character */

    struct aws_huffman_character_coder *coder = test_get_coder();

    for (int i = 0; i < sizeof(code_points) / sizeof(struct huffman_test_code_point); ++i) {
        struct huffman_test_code_point *value = &code_points[i];

        struct aws_huffman_bit_pattern pattern = coder->encode(value->symbol, NULL);

        ASSERT_UINT_EQUALS(value->pattern.pattern, pattern.pattern);
        ASSERT_UINT_EQUALS(value->pattern.num_bits, pattern.num_bits);
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_character_encoder, test_huffman_character_encoder)

static int test_huffman_encoder(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a short url */

    uint8_t output_buffer[encoded_url_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = test_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = url_string_len;
    aws_huffman_coder_state state = aws_huffman_encode(&encoder, url_string, &processed, output_buffer, &output_size);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_EOS_REACHED, state);

    ASSERT_UINT_EQUALS(encoded_url_len, output_size);
    ASSERT_UINT_EQUALS(0, output_buffer[encoded_url_len]);
    ASSERT_BIN_ARRAYS_EQUALS(encoded_url, encoded_url_len, output_buffer, output_size);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder, test_huffman_encoder)

static int test_huffman_encoder_all_code_points(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a sequence of all character values expressable as characters */

    uint8_t output_buffer[encoded_codes_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = test_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = all_codes_len;
    aws_huffman_coder_state state = aws_huffman_encode(&encoder, all_codes, &processed, output_buffer, &output_size);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_EOS_REACHED, state);

    ASSERT_UINT_EQUALS(encoded_codes_len, output_size);
    ASSERT_UINT_EQUALS(0, output_buffer[encoded_codes_len]);
    ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, encoded_codes_len, output_buffer, output_size);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder_all_code_points, test_huffman_encoder_all_code_points)

static int test_huffman_encoder_partial_output(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding when the output buffer size is limited */

    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, test_get_coder());

    uint8_t output_buffer[encoded_codes_len];
    AWS_ZERO_ARRAY(output_buffer);

    static const size_t step_sizes[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

    for (int i = 0; i < sizeof(step_sizes) / sizeof(size_t); ++i) {
        size_t step_size = step_sizes[i];

        const char *current_input = all_codes;
        uint8_t *current_output = output_buffer;
        size_t bytes_written = 0;
        size_t bytes_to_read = all_codes_len;
        for (size_t bytes_to_write = encoded_codes_len; bytes_to_write > 0; ) {

            size_t output_size = bytes_to_write > step_size ? step_size : bytes_to_write;
            size_t processed = bytes_to_read;

            aws_huffman_coder_state state = aws_huffman_encode(&encoder, current_input, &processed, current_output, &output_size);
            (void)state;

            ASSERT_TRUE(output_size > 0);

            bytes_written += output_size;
            bytes_to_read -= processed;
            bytes_to_write -= output_size;
            current_output += output_size;
            current_input += processed;

            ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, bytes_written, output_buffer, bytes_written);
            ASSERT_TRUE(bytes_to_write >= 0);
            ASSERT_UINT_EQUALS(bytes_to_write == 0 ? AWS_HUFFMAN_EOS_REACHED : AWS_HUFFMAN_NEED_MORE_OUTPUT, state);
        }

        ASSERT_UINT_EQUALS(encoded_codes_len, bytes_written);

        ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, encoded_codes_len, output_buffer, bytes_written);
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder_partial_output, test_huffman_encoder_partial_output)

static int test_huffman_character_decoder(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding each character */

    struct aws_huffman_character_coder *coder = test_get_coder();

    for (int i = 0; i < sizeof(code_points) / sizeof(struct huffman_test_code_point); ++i) {
        struct huffman_test_code_point *value = &code_points[i];

        uint32_t bit_pattern = value->pattern.pattern << (32 - value->pattern.num_bits);

        uint16_t out;
        size_t bits_read = coder->decode(bit_pattern, &out, NULL);

        ASSERT_UINT_EQUALS(value->symbol, out);
        ASSERT_UINT_EQUALS(value->pattern.num_bits, bits_read);
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_character_decoder, test_huffman_character_decoder)

static int test_huffman_decoder(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding a short url */

    char output_buffer[url_string_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = test_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = encoded_url_len;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, encoded_url, &processed, output_buffer, &output_size);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_EOS_REACHED, state);
    ASSERT_UINT_EQUALS(url_string_len, output_size);
    ASSERT_UINT_EQUALS(encoded_url_len, processed);
    ASSERT_UINT_EQUALS(output_buffer[url_string_len], 0);
    ASSERT_BIN_ARRAYS_EQUALS(url_string, url_string_len, output_buffer, output_size);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder, test_huffman_decoder)

static int test_huffman_decoder_all_code_points(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding a sequence of all character values expressable as characters */

    char output_buffer[all_codes_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = test_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = encoded_codes_len;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, encoded_codes, &processed, output_buffer, &output_size);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_EOS_REACHED, state);
    ASSERT_UINT_EQUALS(all_codes_len, output_size);
    ASSERT_UINT_EQUALS(encoded_codes_len, processed);
    ASSERT_UINT_EQUALS(output_buffer[all_codes_len], 0);
    ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_buffer, output_size);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_all_code_points, test_huffman_decoder_all_code_points)

static int test_huffman_decoder_partial_input(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding a buffer in chunks */

    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, test_get_coder());

    char output_buffer[150];
    AWS_ZERO_ARRAY(output_buffer);

    static const size_t step_sizes[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

    for (int i = 0; i < sizeof(step_sizes) / sizeof(size_t); ++i) {
        size_t step_size = step_sizes[i];

        uint8_t *current_input = encoded_codes;
        char *current_output = output_buffer;
        size_t bytes_written = 0;
        for (size_t bytes_to_process = encoded_codes_len; bytes_to_process > 0; ) {

            size_t output_size = sizeof(output_buffer) - bytes_written;
            size_t to_process = bytes_to_process > step_size ? step_size : bytes_to_process;

            aws_huffman_coder_state state = aws_huffman_decode(&decoder, current_input, &to_process, current_output, &output_size);

            ASSERT_TRUE(to_process > 0);
            ASSERT_BIN_ARRAYS_EQUALS(all_codes + bytes_written, output_size, output_buffer + bytes_written, output_size);

            bytes_written += output_size;
            bytes_to_process -= to_process;
            current_output += output_size;
            current_input += to_process;

            ASSERT_UINT_EQUALS(bytes_to_process == 0 ? AWS_HUFFMAN_EOS_REACHED : AWS_HUFFMAN_NEED_MORE_DATA, state);
        }

        ASSERT_UINT_EQUALS(all_codes_len, bytes_written);
        ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_buffer, bytes_written);
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_partial_input, test_huffman_decoder_partial_input)

static int test_huffman_decoder_partial_output(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding when the output buffer size is limited */

    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, test_get_coder());

    char output_buffer[150];
    AWS_ZERO_ARRAY(output_buffer);

    static const size_t step_sizes[] = { 2, 4, 8, 16, 32, 64, 128 };

    for (int i = 0; i < sizeof(step_sizes) / sizeof(size_t); ++i) {
        size_t step_size = step_sizes[i];

        uint8_t *current_input = encoded_codes;
        char *current_output = output_buffer;
        size_t bytes_written = 0;
        for (size_t bytes_to_process = encoded_codes_len; bytes_written < all_codes_len; ) {

            size_t output_size = step_size;
            size_t processed = bytes_to_process;

            aws_huffman_coder_state state = aws_huffman_decode(&decoder, current_input, &processed, current_output, &output_size);

            ASSERT_TRUE(output_size > 0);
            ASSERT_BIN_ARRAYS_EQUALS(all_codes + bytes_written, output_size, output_buffer + bytes_written, output_size);

            bytes_written += output_size;
            bytes_to_process -= processed;
            current_output += output_size;
            current_input += processed;

            ASSERT_UINT_EQUALS(bytes_written == all_codes_len ? AWS_HUFFMAN_EOS_REACHED : AWS_HUFFMAN_NEED_MORE_OUTPUT, state);
        }

        ASSERT_UINT_EQUALS(all_codes_len, bytes_written);
        ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_buffer, bytes_written);
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_partial_output, test_huffman_decoder_partial_output)

static int test_huffman_transitive(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a short url and immediately decoding it */

    struct aws_huffman_character_coder *coder = test_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    uint8_t intermediate_buffer[16];
    AWS_ZERO_ARRAY(intermediate_buffer);
    char output_string[16];
    AWS_ZERO_ARRAY(output_string);

    size_t encode_output_size = sizeof(intermediate_buffer);
    size_t processed = url_string_len;
    aws_huffman_encode(&encoder, url_string, &processed, intermediate_buffer, &encode_output_size);

    ASSERT_UINT_EQUALS(encode_output_size, encoded_url_len);
    ASSERT_UINT_EQUALS(intermediate_buffer[encoded_url_len], 0);

    size_t decode_output_size = sizeof(output_string);
    processed = encode_output_size;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, intermediate_buffer, &processed, output_string, &decode_output_size);

    ASSERT_UINT_EQUALS(AWS_HUFFMAN_EOS_REACHED, state);
    ASSERT_UINT_EQUALS(url_string_len, decode_output_size);
    ASSERT_UINT_EQUALS(encode_output_size, processed);
    ASSERT_UINT_EQUALS(output_string[url_string_len], 0);

    ASSERT_BIN_ARRAYS_EQUALS(url_string, url_string_len, output_string, decode_output_size);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_transitive, test_huffman_transitive)

static int test_huffman_transitive_all_code_points(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a sequence of all character values expressable as characters and immediately decoding it */

    struct aws_huffman_character_coder *coder = test_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    uint8_t intermediate_buffer[encoded_codes_len + 1];
    AWS_ZERO_ARRAY(intermediate_buffer);
    char output_string[all_codes_len + 1];
    AWS_ZERO_ARRAY(output_string);

    size_t encode_output_size = sizeof(intermediate_buffer);
    size_t processed = all_codes_len;
    aws_huffman_encode(&encoder, all_codes, &processed, intermediate_buffer, &encode_output_size);

    ASSERT_UINT_EQUALS(encode_output_size, encoded_codes_len);
    ASSERT_UINT_EQUALS(intermediate_buffer[encoded_codes_len], 0);

    size_t output_size = sizeof(output_string);
    processed = encode_output_size;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, intermediate_buffer, &processed, output_string, &output_size);

    ASSERT_UINT_EQUALS(AWS_HUFFMAN_EOS_REACHED, state);
    ASSERT_UINT_EQUALS(all_codes_len, output_size);
    ASSERT_UINT_EQUALS(encode_output_size, processed);
    ASSERT_UINT_EQUALS(output_string[all_codes_len], 0);

    ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_string, output_size);

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_transitive_all_code_points, test_huffman_transitive_all_code_points)
