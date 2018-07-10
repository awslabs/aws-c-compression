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

#include <aws/compression/exports.h>

#include <aws/common/common.h>

#include <stddef.h>
#include <stdint.h>

/**
 * Represents an encoded bit pattern
 */
struct aws_huffman_code {
    /**
     * The value of the bit pattern
     * \note The pattern is stored in the least significant bits
     */
    uint32_t pattern;
    /** The number of bits in pattern to use */
    uint8_t num_bits;
};

/**
 * Function used to encode a single character to an aws_huffman_code
 *
 * \param[in] symbol    The symbol to encode
 * \param[in] userdata  Optional userdata (aws_huffman_symbol_coder.userdata)
 *
 * \returns The code representing the symbol
 */
typedef struct aws_huffman_code (*aws_huffman_symbol_encoder)(uint16_t symbol, void *userdata);
/**
 * Function used to decode a bit pattern into a character
 *
 * \param[in]   code        The bits to attemt to decode a symbol from
 * \param[out]  symbol      The symbol found. Do not write to if no valid symbol found
 * \param[in]   userdata    Optional userdata (aws_huffman_symbol_coder.userdata)
 *
 * \returns The number of bits read from code
 */
typedef uint8_t (*aws_huffman_symbol_decoder)(uint32_t code, uint16_t *symbol, void *userdata);

/**
 * Structure used to define how characters are encoded and decoded
 *
 * This struct may be provided by the code generator
 */
struct aws_huffman_symbol_coder {
    aws_huffman_symbol_encoder encode;
    aws_huffman_symbol_decoder decode;
    uint16_t eos_symbol;
    void *userdata;
};

/**
 * Structure used for persistent encoding.
 * Allows for reading from or writing to incomplete buffers.
 */
struct aws_huffman_encoder {
    struct aws_huffman_symbol_coder *coder;
    struct aws_huffman_code overflow_bits;
};

/**
 * Structure used for persistent decoding.
 * Allows for reading from or writing to incomplete buffers.
 */
struct aws_huffman_decoder {
    struct aws_huffman_symbol_coder *coder;
    uint64_t working_bits;
    uint8_t num_bits;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a encoder object with a character coder.
 */
AWS_COMPRESSION_API void aws_huffman_encoder_init(struct aws_huffman_encoder *encoder, struct aws_huffman_symbol_coder *coder);

/**
 * Resets a decoder for use with a new binary stream
 */
AWS_COMPRESSION_API void aws_huffman_encoder_reset(struct aws_huffman_encoder *encoder);

/**
 * Initialize a decoder object with a character coder.
 */
AWS_COMPRESSION_API void aws_huffman_decoder_init(struct aws_huffman_decoder *decoder, struct aws_huffman_symbol_coder *coder);

/**
 * Resets a decoder for use with a new binary stream
 */
AWS_COMPRESSION_API void aws_huffman_decoder_reset(struct aws_huffman_decoder *decoder);

/**
 * Encode a character buffer into the output buffer.
 *
 * \param[in]       encoder         The encoder object to use
 * \param[in]       to_encode       The character buffer to encode
 * \param[in,out]   length          In: The length of to_decode Out: The number of bytes read from to_encode
 * \param[in]       output          The buffer to write encoded bytes to
 * \param[in,out]   output_size     In: The size of output Out: The number of bytes written to output
 *
 * \return AWS_ERROR_SUCCESS if encoding is successful, otherwise the code for the error that occured
 */
AWS_COMPRESSION_API aws_common_error aws_huffman_encode(struct aws_huffman_encoder *encoder, const char *to_encode, size_t *length, uint8_t *output, size_t *output_size);

/**
 * Decodes a byte buffer into the provided character array.
 *
 * \param[in]       decoder         The decoder object to use
 * \param[in]       to_decode       The encoded byte buffer to read from
 * \param[in,out]   length          In: The length of to_decode Out: The number of bytes read from to_encode
 * \param[in]       output          The buffer to write decoded characters to
 * \param[in,out]   output_size     In: The size of output Out: The number of bytes written to output
 *
 * \return AWS_ERROR_SUCCESS if encoding is successful, otherwise the code for the error that occured
 */
AWS_COMPRESSION_API aws_common_error aws_huffman_decode(struct aws_huffman_decoder *decoder, const uint8_t *to_decode, size_t *length, char *output, size_t *output_size);

#ifdef __cplusplus
}
#endif

#endif /* AWS_COMPRESSION_HUFFMAN_H */
