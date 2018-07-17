#ifndef AWS_COMPRESSION_TESTING_HUFFMAN_INL
#define AWS_COMPRESSION_TESTING_HUFFMAN_INL

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

#ifdef _WIN32
#include <malloc.h>
#define alloca _alloca
#endif /* _WIN32 */

int huffman_test_transitive(struct aws_huffman_symbol_coder *coder, const char *input, size_t size, const char **error_string) {

    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    const size_t intermediate_buffer_size = size * 2;
    uint8_t *intermediate_buffer = alloca(intermediate_buffer_size);
    memset(intermediate_buffer, 0, intermediate_buffer_size);
    char *output_buffer = alloca(size);
    memset(output_buffer, 0, size);

    size_t encode_input_size = size;
    size_t encode_output_size = intermediate_buffer_size;
    int result = aws_huffman_encode(&encoder, input, &encode_input_size, intermediate_buffer, &encode_output_size);
    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_encode failed";
        return AWS_OP_ERR;
    }
    if (encode_input_size != size) {
        *error_string = "not all data encoded";
        return AWS_OP_ERR;
    }
    if (encode_output_size > intermediate_buffer_size) {
        *error_string = "too much data encoded";
        return AWS_OP_ERR;
    }

    size_t decode_input_size = encode_output_size;
    size_t decode_output_size = size;
    result = aws_huffman_decode(&decoder, intermediate_buffer, &decode_input_size, output_buffer, &decode_output_size);
    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_decode failed";
        return AWS_OP_ERR;
    }
    if (decode_input_size != encode_output_size) {
        *error_string = "not all encoded data was decoded";
        return AWS_OP_ERR;
    }
    if (decode_output_size != size) {
        *error_string = "decode output size incorrect";
        return AWS_OP_ERR;
    }
    if (memcmp(input, output_buffer, decode_output_size) != 0) {
        *error_string = "decoded data does not match input data";
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

int huffman_test_transitive_chunked(struct aws_huffman_symbol_coder *coder, const char *input, size_t size, size_t output_chunk_size, const char **error_string) {

    struct aws_huffman_encoder encoder;
    aws_huffman_encoder_init(&encoder, coder);
    struct aws_huffman_decoder decoder;
    aws_huffman_decoder_init(&decoder, coder);

    const size_t intermediate_buffer_size = size * 2;
    uint8_t *intermediate_buffer = alloca(intermediate_buffer_size);
    memset(intermediate_buffer, 0, intermediate_buffer_size);
    char *output_buffer = alloca(size);
    memset(output_buffer, 0, size);

    int result = AWS_OP_SUCCESS;
    size_t encode_output_size = 0;

    {
        const char *current_input = input;
        uint8_t *current_output = intermediate_buffer;
        size_t bytes_to_process = size;
        do {

            size_t output_size = output_chunk_size;
            size_t processed = bytes_to_process;

            result = aws_huffman_encode(&encoder, current_input, &processed, current_output, &output_size);

            if (output_size == 0) {
                *error_string = "encode didn't write any data";
                return AWS_OP_ERR;
            }

            encode_output_size += output_size;
            bytes_to_process -= processed;
            current_output += output_size;
            current_input += processed;

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
    if (encode_output_size > intermediate_buffer_size) {
        *error_string = "too much data encoded";
        return AWS_OP_ERR;
    }

    size_t decode_output_size = 0;

    {
        uint8_t *current_input = intermediate_buffer;
        char *current_output = output_buffer;
        size_t bytes_to_process = encode_output_size;
        do {

            size_t output_size = output_chunk_size;
            size_t processed = bytes_to_process;

            result = aws_huffman_decode(&decoder, current_input, &processed, current_output, &output_size);

            if (output_size == 0) {
                *error_string = "decode didn't write any data";
                return AWS_OP_ERR;
            }

            decode_output_size += output_size;
            bytes_to_process -= processed;
            current_output += output_size;
            current_input += processed;

            int error = aws_last_error();   (void)error;
            if (result != AWS_OP_SUCCESS && aws_last_error() != AWS_ERROR_SHORT_BUFFER) {
                *error_string = "decode returned wrong error code";
                return AWS_OP_ERR;
            }
        }  while (result != AWS_OP_SUCCESS);
    }

    if (result != AWS_OP_SUCCESS) {
        *error_string = "aws_huffman_decode failed";
        return AWS_OP_ERR;
    }
    if (decode_output_size != size) {
        *error_string = "decode output size incorrect";
        return AWS_OP_ERR;
    }
    if (memcmp(input, output_buffer, decode_output_size) != 0) {
        *error_string = "decoded data does not match input data";
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

#endif /* AWS_COMPRESSION_TESTING_HUFFMAN_INL */
