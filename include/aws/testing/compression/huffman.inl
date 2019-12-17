#ifndef AWS_TESTING_COMPRESSION_HUFFMAN_INL
#define AWS_TESTING_COMPRESSION_HUFFMAN_INL

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

/**
 * See aws/testing/compression/huffman.h for docs.
 */

#include <aws/common/common.h>
#include <aws/common/byte_buf.h>

int huffman_test_transitive(
    struct aws_huffman_symbol_coder *coder,
    const char *input,
    size_t size,
    size_t encoded_size,
    const char **error_string) {

    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    const size_t intermediate_buffer_size = size * 2;
    AWS_VARIABLE_LENGTH_ARRAY(uint8_t, intermediate_buffer, intermediate_buffer_size);
    memset(intermediate_buffer, 0, intermediate_buffer_size);
    AWS_VARIABLE_LENGTH_ARRAY(char, output_buffer, size);
    memset(output_buffer, 0, size);

    struct aws_byte_cursor to_encode = aws_byte_cursor_from_array((uint8_t *)input, size);
    struct aws_byte_buf intermediate_buf = aws_byte_buf_from_empty_array(intermediate_buffer, intermediate_buffer_size);
    struct aws_byte_buf output_buf = aws_byte_buf_from_empty_array(output_buffer, size);

    int result = aws_huffman_encode(&encoder, &to_encode, &intermediate_buf);

    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_encode failed";
        return AWS_OP_ERR;
    }
    if (to_encode.len != 0) {
        *error_string = "not all data encoded";
        return AWS_OP_ERR;
    }
    if (encoded_size && intermediate_buf.len != encoded_size) {
        *error_string = "encoded length is incorrect";
        return AWS_OP_ERR;
    }

    struct aws_byte_cursor intermediate_cur = aws_byte_cursor_from_buf(&intermediate_buf);
    result = aws_huffman_decode(&decoder, &intermediate_cur, &output_buf);

    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_decode failed";
        return AWS_OP_ERR;
    }
    if (intermediate_cur.len != 0) {
        *error_string = "not all encoded data was decoded";
        return AWS_OP_ERR;
    }
    if (output_buf.len != size) {
        *error_string = "decode output size incorrect";
        return AWS_OP_ERR;
    }
    if (memcmp(input, output_buffer, size) != 0) {
        *error_string = "decoded data does not match input data";
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

int huffman_test_transitive_chunked(
    struct aws_huffman_symbol_coder *coder,
    const char *input,
    size_t size,
    size_t encoded_size,
    size_t output_chunk_size,
    const char **error_string) {

    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    const size_t intermediate_buffer_size = size * 2;
    AWS_VARIABLE_LENGTH_ARRAY(uint8_t, intermediate_buffer, intermediate_buffer_size);
    memset(intermediate_buffer, 0, intermediate_buffer_size);
    AWS_VARIABLE_LENGTH_ARRAY(char, output_buffer, size);
    memset(output_buffer, 0, size);

    struct aws_byte_cursor to_encode = aws_byte_cursor_from_array(input, size);
    struct aws_byte_buf intermediate_buf = aws_byte_buf_from_empty_array(intermediate_buffer, (size_t)-1);
    intermediate_buf.capacity = 0;
    struct aws_byte_buf output_buf = aws_byte_buf_from_empty_array(output_buffer, (size_t)-1);
    output_buf.capacity = 0;

    int result = AWS_OP_SUCCESS;

    {
        do {
            const size_t previous_intermediate_len = intermediate_buf.len;

            intermediate_buf.capacity += output_chunk_size;
            result = aws_huffman_encode(&encoder, &to_encode, &intermediate_buf);

            if (intermediate_buf.len == previous_intermediate_len) {
                *error_string = "encode didn't write any data";
                return AWS_OP_ERR;
            }

            if (result != AWS_OP_SUCCESS && aws_last_error() != AWS_ERROR_SHORT_BUFFER) {
                *error_string = "encode returned wrong error code";
                return AWS_OP_ERR;
            }
        } while (result != AWS_OP_SUCCESS);
    }

    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_encode failed";
        return AWS_OP_ERR;
    }
    if (intermediate_buf.len > intermediate_buffer_size) {
        *error_string = "too much data encoded";
        return AWS_OP_ERR;
    }
    if (encoded_size && intermediate_buf.len != encoded_size) {
        *error_string = "encoded length is incorrect";
        return AWS_OP_ERR;
    }

    struct aws_byte_cursor intermediate_cur = aws_byte_cursor_from_buf(&intermediate_buf);

    {
        do {
            const size_t previous_output_len = output_buf.len;

            output_buf.capacity += output_chunk_size;
            if (output_buf.capacity > size) {
                output_buf.capacity = size;
            }

            result = aws_huffman_decode(&decoder, &intermediate_cur, &output_buf);

            if (output_buf.len == previous_output_len) {
                *error_string = "decode didn't write any data";
                return AWS_OP_ERR;
            }

            if (result != AWS_OP_SUCCESS && aws_last_error() != AWS_ERROR_SHORT_BUFFER) {
                *error_string = "decode returned wrong error code";
                return AWS_OP_ERR;
            }
        } while (result != AWS_OP_SUCCESS);
    }

    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_decode failed";
        return AWS_OP_ERR;
    }
    if (output_buf.len != size) {
        *error_string = "decode output size incorrect";
        return AWS_OP_ERR;
    }
    if (memcmp(input, output_buffer, size) != 0) {
        *error_string = "decoded data does not match input data";
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

#endif /* AWS_TESTING_COMPRESSION_HUFFMAN_INL */
