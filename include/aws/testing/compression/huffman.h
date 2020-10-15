#ifndef AWS_TESTING_COMPRESSION_HUFFMAN_H
#define AWS_TESTING_COMPRESSION_HUFFMAN_H

/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <aws/testing/aws_test_harness.h>

#include <aws/compression/huffman.h>

#include <aws/common/byte_buf.h>
#include <aws/common/common.h>

#include <stddef.h>

/**
 * The intended use of file is to allow testing of huffman character coders.
 * By doing the following, you can ensure the output of encoders decoders are
 * correct:
 *
 * \code{c}
 * static struct huffman_test_code_point code_points[] = {
 * #include "test_huffman_static_table.def"
 * };
 * \endcode
 *
 * You may then iterate over each code point in the array, and test the
 * following (pseudo-code):
 *
 * \code{c} for (cp in code_points) {
 *     AWS_ASSERT(my_coder->encode(cp.symbol) == cp.pattern);
 *     AWS_ASSERT(my_coder->decode(cp.pattern) == cp.symbol);
 * }
 * \endcode
 */

/**
 * Structure containing all relevant information about a code point
 */
struct huffman_test_code_point {
    uint8_t symbol;
    struct aws_huffman_code code;
};

/**
 * Macro to be used when including a table def file, populates an array of
 * huffman_test_code_points
 */
#define HUFFMAN_CODE(psymbol, pbit_string, pbit_pattern, pnum_bits)                                                    \
    {                                                                                                                  \
        .symbol = (psymbol),                                                                                           \
        .code =                                                                                                        \
            {                                                                                                          \
                .pattern = (pbit_pattern),                                                                             \
                .num_bits = (pnum_bits),                                                                               \
            },                                                                                                         \
    },

/**
 * Function to test a huffman coder to ensure the transitive property applies
 * (input == decode(incode(input)))
 *
 * \param[in]   coder           The symbol coder to test
 * \param[in]   input           The buffer to test
 * \param[in]   size            The size of input
 * \param[in]   encoded_size    The length of the encoded buffer. Pass 0 to skip check.
 * \param[out]  error_string    In case of failure, the error string to report
 *
 * \return AWS_OP_SUCCESS on success, AWS_OP_FAILURE on failure (error_string
 * will be set)
 */
static int huffman_test_transitive(
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

/**
 * Function to test a huffman coder to ensure the transitive property applies
 * when doing partial encodes/decodes (input == decode(incode(input)))
 *
 * \param[in]   coder               The symbol coder to test
 * \param[in]   input               The buffer to test
 * \param[in]   size                The size of input
 * \param[in]   encoded_size        The length of the encoded buffer. Pass 0 to skip check.
 * \param[in]   output_chunk_size   The amount of output to write at once
 * \param[out]  error_string        In case of failure, the error string to
 * report
 *
 * \return AWS_OP_SUCCESS on success, AWS_OP_FAILURE on failure (error_string
 * will be set)
 */
static int huffman_test_transitive_chunked(
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

#endif /* AWS_TESTING_COMPRESSION_HUFFMAN_H */
