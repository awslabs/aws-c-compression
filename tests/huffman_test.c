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

/* Exported by generated file */
struct aws_huffman_character_coder *hpack_get_coder();

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

/* Useful data for testing */
static const char url_string[] = "www.example.com";
static size_t url_string_len = sizeof(url_string) - 1;
static uint8_t encoded_url[] = { 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff, 0xff, 0xff, 0xff };
static size_t encoded_url_len = 15;

static const char all_codes[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static size_t all_codes_len = sizeof(all_codes) - 1;
static uint8_t encoded_codes[] = {
    0x53, 0xf8, 0xfe, 0x7f, 0xeb, 0xff, 0x2a, 0xfc, 0x7f, 0xaf, 0xeb, 0xfb, 0xf9, 0xff, 0x7f, 0x4b, 0x2e, 0xc0, 0x02, 0x26, 0x5a,
    0x6d, 0xc7, 0x5e, 0x7e, 0xe7, 0xdf, 0xff, 0xc8, 0x3f, 0xef, 0xfc, 0xff, 0xd4, 0x37, 0x6f, 0x5f, 0xc1, 0x87, 0x16, 0x3c, 0x99,
    0x73, 0x67, 0xd1, 0xa7, 0x56, 0xbd, 0x9b, 0x77, 0x6f, 0xe1, 0xc7, 0x97, 0xe7, 0x3f, 0xdf, 0xfd, 0xff, 0xff, 0x0f, 0xfe, 0x7f,
    0xf9, 0x17, 0xff, 0xd1, 0xc6, 0x49, 0x0b, 0x2c, 0xd3, 0x9b, 0xa7, 0x5a, 0x29, 0xa8, 0xf5, 0xf6, 0xb1, 0x9, 0xb7, 0xbf, 0x8f,
    0x3e, 0xbd, 0xff, 0xfe, 0xff, 0x9f, 0xfe, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xff,
};
static size_t encoded_codes_len = sizeof(encoded_codes);

static int test_huffman_character_encoder(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding each character */

    struct aws_huffman_character_coder *coder = hpack_get_coder();

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
    /* Test encoding a short url */

    uint8_t output_buffer[encoded_url_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = hpack_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = 0;
    aws_huffman_encode(&encoder, url_string, url_string_len, output_buffer, &output_size, &processed);

    ASSERT_UINT_EQUALS(encoded_url_len, output_size, "Encoder wrote incorrect number bytes");
    ASSERT_UINT_EQUALS(0, output_buffer[encoded_url_len], "Encoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(encoded_url, encoded_url_len, output_buffer, output_size, "Data written is incorrect!");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder, test_huffman_encoder)

static int test_huffman_encoder_all_code_points(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a sequence of all character values expressable as characters */

    uint8_t output_buffer[encoded_codes_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = hpack_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = 0;
    aws_huffman_encode(&encoder, all_codes, all_codes_len, output_buffer, &output_size, &processed);

    ASSERT_UINT_EQUALS(encoded_codes_len, output_size, "Encoder wrote incorrect number bytes");
    ASSERT_UINT_EQUALS(0, output_buffer[encoded_codes_len], "Encoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, encoded_codes_len, output_buffer, output_size, "Data written is incorrect!");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder_all_code_points, test_huffman_encoder_all_code_points)

static int test_huffman_encoder_partial_output(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding when the output buffer size is limited */

    uint8_t output_buffer[encoded_codes_len];
    AWS_ZERO_ARRAY(output_buffer);

    static const size_t step_sizes[] = { 4, 8, 16, 32, 64, 128 };

    for (int i = 0; i < sizeof(step_sizes) / sizeof(size_t); ++i) {
        size_t step_size = step_sizes[i];

        struct aws_huffman_encoder encoder;
        aws_huffman_encoder_init(&encoder, hpack_get_coder());

        const char *current_input = all_codes;
        uint8_t *current_output = output_buffer;
        size_t bytes_written = 0;
        size_t bytes_to_read = all_codes_len;
        for (int bytes_to_write = encoded_codes_len; bytes_to_write > 0; ) {

            size_t output_size = bytes_to_write > step_size ? step_size : bytes_to_write;
            size_t processed = 0;

            aws_huffman_coder_state state = aws_huffman_encode(&encoder, current_input, bytes_to_read, current_output, &output_size, &processed);
            (void)state;

            ASSERT_TRUE(output_size > 0, "0 bytes written");

            bytes_written += output_size;
            bytes_to_read -= processed;
            bytes_to_write -= output_size;
            current_output += output_size;
            current_input += processed;

            ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, bytes_written, output_buffer, bytes_written, "Incorrect full byte buffer");

            ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, bytes_written, output_buffer, bytes_written);
            ASSERT_TRUE(bytes_to_write >= 0, "Encoder wrote too many bytes");
            ASSERT_UINT_EQUALS(bytes_to_write == 0 ? AWS_HUFFMAN_DECODE_EOS_REACHED : AWS_HUFFMAN_DECODE_NEED_MORE_OUTPUT, state, "Encoder ended in the wrong state step_size: %u", step_size);
        }

        ASSERT_UINT_EQUALS(encoded_codes_len, bytes_written);

        ASSERT_BIN_ARRAYS_EQUALS(encoded_codes, encoded_codes_len, output_buffer, bytes_written, "Incorrect full byte buffer");
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_encoder_partial_output, test_huffman_encoder_partial_output)

static int test_huffman_character_decoder(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding each character */

    struct aws_huffman_character_coder *coder = hpack_get_coder();

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
    /* Test decoding a short url */

    char output_buffer[url_string_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = hpack_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = 0;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, encoded_url, encoded_url_len, output_buffer, &output_size, &processed);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(url_string_len, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(encoded_url_len, processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_buffer[url_string_len], 0, "Decoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(url_string, url_string_len, output_buffer, output_size, "Data written is incorrect");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder, test_huffman_decoder)

static int test_huffman_decoder_all_code_points(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding a sequence of all character values expressable as characters */

    char output_buffer[all_codes_len + 1];
    AWS_ZERO_ARRAY(output_buffer);

    struct aws_huffman_character_coder *coder = hpack_get_coder();
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    size_t output_size = sizeof(output_buffer);
    size_t processed = 0;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, encoded_codes, encoded_codes_len, output_buffer, &output_size, &processed);
    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(all_codes_len, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(encoded_codes_len, processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_buffer[all_codes_len], 0, "Decoder wrote too far!");
    ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_buffer, output_size, "Data written is incorrect");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_all_code_points, test_huffman_decoder_all_code_points)

static int test_huffman_decoder_partial_input(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding a buffer in chunks */

    char output_buffer[150];
    AWS_ZERO_ARRAY(output_buffer);

    static const size_t step_sizes[] = { 8, 16, 32, 64, 128 };

    for (int i = 0; i < sizeof(step_sizes) / sizeof(size_t); ++i) {
        size_t step_size = step_sizes[i];

        struct aws_huffman_decoder decoder;
        aws_huffman_decoder_init(&decoder, hpack_get_coder());

        uint8_t *current_input = encoded_codes;
        char *current_output = output_buffer;
        size_t bytes_written = 0;
        for (size_t bytes_to_process = encoded_codes_len; bytes_to_process > 0; ) {

            size_t output_size = sizeof(output_buffer) - bytes_written;
            size_t processed = 0;
            size_t to_process = bytes_to_process > step_size ? step_size : bytes_to_process;

            aws_huffman_coder_state state = aws_huffman_decode(&decoder, current_input, to_process, current_output, &output_size, &processed);

            ASSERT_TRUE(processed > 0, "0 bytes processed");
            ASSERT_BIN_ARRAYS_EQUALS(all_codes + bytes_written, output_size, output_buffer + bytes_written, output_size, "Incorrect bytes written");

            bytes_written += output_size;
            bytes_to_process -= processed;
            current_output += output_size;
            current_input += processed;

            ASSERT_UINT_EQUALS(bytes_to_process == 0 ? AWS_HUFFMAN_DECODE_EOS_REACHED : AWS_HUFFMAN_DECODE_NEED_MORE_DATA, state, "Decoder ended in the wrong state");
        }

        ASSERT_UINT_EQUALS(all_codes_len, bytes_written);
        ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_buffer, bytes_written, "Incorrect full byte buffer");
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_partial_input, test_huffman_decoder_partial_input)

static int test_huffman_decoder_partial_output(struct aws_allocator *allocator, void *user_data) {
    /* Test decoding when the output buffer size is limited */

    char output_buffer[150];
    AWS_ZERO_ARRAY(output_buffer);

    static const size_t step_sizes[] = { 2, 4, 8, 16, 32, 64, 128 };

    for (int i = 0; i < sizeof(step_sizes) / sizeof(size_t); ++i) {
        size_t step_size = step_sizes[i];

        struct aws_huffman_decoder decoder;
        aws_huffman_decoder_init(&decoder, hpack_get_coder());

        uint8_t *current_input = encoded_codes;
        char *current_output = output_buffer;
        size_t bytes_written = 0;
        for (size_t bytes_to_process = encoded_codes_len; bytes_to_process > 0; ) {

            size_t output_size = step_size;
            size_t processed = 0;

            aws_huffman_coder_state state = aws_huffman_decode(&decoder, current_input, bytes_to_process, current_output, &output_size, &processed);

            ASSERT_TRUE(processed > 0, "0 bytes processed");
            ASSERT_TRUE(output_size > 0, "0 bytes written");
            ASSERT_BIN_ARRAYS_EQUALS(all_codes + bytes_written, output_size, output_buffer + bytes_written, output_size, "Incorrect bytes written");

            bytes_written += output_size;
            bytes_to_process -= processed;
            current_output += output_size;
            current_input += processed;

            ASSERT_UINT_EQUALS(bytes_to_process == 0 ? AWS_HUFFMAN_DECODE_EOS_REACHED : AWS_HUFFMAN_DECODE_NEED_MORE_OUTPUT, state, "Decoder ended in the wrong state");
        }

        ASSERT_UINT_EQUALS(all_codes_len, bytes_written);
        ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_buffer, bytes_written, "Incorrect full byte buffer");
    }

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_decoder_partial_output, test_huffman_decoder_partial_output)

static int test_huffman_transitive(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a short url and immediately decoding it */

    struct aws_huffman_character_coder *coder = hpack_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    uint8_t intermediate_buffer[16];
    AWS_ZERO_ARRAY(intermediate_buffer);
    char output_string[16];
    AWS_ZERO_ARRAY(output_string);

    size_t encode_output_size = sizeof(intermediate_buffer);
    size_t processed = 0;
    aws_huffman_encode(&encoder, url_string, url_string_len, intermediate_buffer, &encode_output_size, &processed);

    ASSERT_UINT_EQUALS(encode_output_size, 15, "Encoder wrote too many bytes");
    ASSERT_UINT_EQUALS(intermediate_buffer[15], 0, "Encoder wrote too far!");

    size_t decode_output_size = sizeof(output_string);
    processed = 0;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, intermediate_buffer, encode_output_size, output_string, &decode_output_size, &processed);

    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(url_string_len, encode_output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(encode_output_size, processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_string[15], 0, "Decoder wrote too far!");

    ASSERT_BIN_ARRAYS_EQUALS(url_string, url_string_len, output_string, decode_output_size, "Strings at begin and end don't match");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_transitive, test_huffman_transitive)

static int test_huffman_transitive_all_code_points(struct aws_allocator *allocator, void *user_data) {
    /* Test encoding a sequence of all character values expressable as characters and immediately decoding it */

    struct aws_huffman_character_coder *coder = hpack_get_coder();
    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    uint8_t intermediate_buffer[98];
    AWS_ZERO_ARRAY(intermediate_buffer);
    char output_string[all_codes_len + 1];
    AWS_ZERO_ARRAY(output_string);

    size_t encode_output_size = sizeof(intermediate_buffer);
    size_t processed = 0;
    aws_huffman_encode(&encoder, all_codes, all_codes_len, intermediate_buffer, &encode_output_size, &processed);

    ASSERT_UINT_EQUALS(encode_output_size, 97, "Encoder wrote too many bytes");
    ASSERT_UINT_EQUALS(intermediate_buffer[97], 0, "Encoder wrote too far!");

    size_t output_size = sizeof(output_string);
    processed = 0;
    aws_huffman_coder_state state = aws_huffman_decode(&decoder, intermediate_buffer, encode_output_size, output_string, &output_size, &processed);

    ASSERT_UINT_EQUALS(AWS_HUFFMAN_DECODE_EOS_REACHED, state, "Decoder ended in the wrong state");
    ASSERT_UINT_EQUALS(all_codes_len, output_size, "Decoder wrote too many bytes");
    ASSERT_UINT_EQUALS(encode_output_size, processed, "Decoder read too few/many bytes");
    ASSERT_UINT_EQUALS(output_string[all_codes_len], 0, "Decoder wrote too far!");

    ASSERT_BIN_ARRAYS_EQUALS(all_codes, all_codes_len, output_string, output_size, "Strings at begin and end don't match");

    return AWS_OP_SUCCESS;
}
AWS_TEST_CASE(huffman_transitive_all_code_points, test_huffman_transitive_all_code_points)
