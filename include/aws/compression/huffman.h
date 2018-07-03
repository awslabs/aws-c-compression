#ifndef AWS_COMPRESSION_HUFFMAN_H
#define AWS_COMPRESSION_HUFFMAN_H

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

#include <stddef.h>
#include <stdint.h>

/**
 * Represents an encoded bit pattern
 */
struct aws_huffman_bit_pattern {
    /**
     * The value of the bit pattern
     * \note The pattern is stored in the bottom bits
     */
    uint32_t pattern;
    /** The number of bits in pattern to use */
    uint8_t num_bits;
};

/**
 * Function used to encode a single character to an aws_bit_pattern
 */
typedef struct aws_huffman_bit_pattern (*aws_huffman_character_encoder)(uint16_t symbol, void *userdata);
/**
 * Function used to decode a bit pattern into a character
 */
typedef size_t (*aws_huffman_character_decoder)(uint32_t bit_pattern, uint16_t *symbol, void *userdata);

/**
 * Structure used to define how characters are encoded and decoded
 *
 * This struct may be provided by the code generator
 */
struct aws_huffman_character_coder {
    aws_huffman_character_encoder encode;
    aws_huffman_character_decoder decode;
    uint16_t eos_symbol;
    void *userdata;
};

/**
 * Structure used for persistent encoding.
 * Allows for reading from or writing to incomplete buffers.
 */
struct aws_huffman_encoder {
    struct aws_huffman_character_coder *coder;
    struct aws_huffman_bit_pattern working_bits;
    unsigned eos_written : 1;
};

/**
 * Structure used for persistent decoding.
 * Allows for reading from or writing to incomplete buffers.
 */
struct aws_huffman_decoder {
    struct aws_huffman_character_coder *coder;
    uint8_t bit_pos;
};

/**
 * The state the decoder is currently in.
 */
typedef enum aws_huffman_coder_state {
    /** The stream has successfully decoded */
    AWS_HUFFMAN_DECODE_EOS_REACHED,
    /** More input data is needed */
    AWS_HUFFMAN_DECODE_NEED_MORE_DATA,
    /** More input data is needed */
    AWS_HUFFMAN_DECODE_NEED_MORE_OUTPUT,
    /** An error occured while decoding */
    AWS_HUFFMAN_DECODE_ERROR
} aws_huffman_coder_state;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a encoder object with a character coder.
 */
void aws_huffman_encoder_init(struct aws_huffman_encoder *encoder, struct aws_huffman_character_coder *coder);

/**
 * Initialize a decoder object with a character coder.
 */
void aws_huffman_decoder_init(struct aws_huffman_decoder *decoder, struct aws_huffman_character_coder *coder);

/**
 * Encode a character buffer into the output buffer.
 *
 * \param[in]       encoder         The encoder object to use
 * \param[in]       to_encode       The character buffer to encode
 * \param[in]       length          The length of to_encode
 * \param[in]       output          The buffer to write encoded bytes to
 * \param[in,out]   output_size     In: The size of output Out: The number of bytes written to output
 * \param[out]      processed       The number of bytes read from to_encode before reaching the end or running out of output space
 *
 * \return The current state of the encoder \see aws_huffman_coder_state
 */
aws_huffman_coder_state aws_huffman_encode(struct aws_huffman_encoder *encoder, const char *to_encode, size_t length, uint8_t *output, size_t *output_size, size_t *processed);

/**
 * Decodes a byte buffer into the provided character array.
 *
 * \param[in]       decoder         The decoder object to use
 * \param[in]       to_decode       The encoded byte buffer to read from
 * \param[in]       length          The length of to_decode
 * \param[in]       output          The buffer to write decoded characters to
 * \param[in,out]   output_size     In: The size of output Out: The number of bytes written to output
 * \param[out]      processed       The number of bytes read from to_decode before reaching the end or running out of output space
 *
 * \return The current state of the decoder \see aws_huffman_coder_state
 */
aws_huffman_coder_state aws_huffman_decode(struct aws_huffman_decoder *decoder, const uint8_t *to_decode, size_t length, char *output, size_t *output_size, size_t *processed);

#ifdef __cplusplus
}
#endif

#endif // AWS_COMPRESSION_HUFFMAN_H
