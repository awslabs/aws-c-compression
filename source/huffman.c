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

#include <assert.h>

size_t aws_huffman_encode(struct aws_huffman_coder *coder, const char *to_encode, size_t length, uint8_t *output) {
    assert(coder);
    assert(to_encode);
    assert(output);

    /* Counters for how far into the output we currently are */
    size_t byte_pos = 0;
    uint8_t bit_pos = 8;

    /* Used to avoid writing out one bit at a time */
    uint8_t working = 0;

#define WRITE_CHARACTER(char_value)                                                             \
    do {                                                                                        \
        struct aws_huffman_bit_pattern code_point = coder->encode(char_value, coder->userdata); \
        for (int bit_idx = code_point.num_bits - 1; bit_idx >= 0; --bit_idx) {                  \
            uint8_t encoded_bit = (uint8_t)((code_point.pattern >> bit_idx) & 0x01);            \
            bit_pos--;                                                                          \
            encoded_bit <<= bit_pos;                                                            \
            working |= encoded_bit;                                                             \
            if (bit_pos == 0) {                                                                 \
                output[byte_pos] = working;                                                     \
                byte_pos++;                                                                     \
                bit_pos = 8;                                                                    \
                working = 0;                                                                    \
            }                                                                                   \
        }                                                                                       \
    } while (0)

    for (size_t i = 0; i < length; ++i) {
        WRITE_CHARACTER(to_encode[i]);
    }
    WRITE_CHARACTER(coder->eos_symbol);

    if (bit_pos < 8) {
        /* Pad the rest out with 1s */
        uint8_t padding_mask = UINT8_MAX >> (8 - bit_pos);
        working |= padding_mask;
        output[byte_pos++] = working;
    }

    return byte_pos;
}

aws_huffman_decoder_state aws_huffman_decode(struct aws_huffman_coder *coder, const uint8_t *buffer, size_t len, char *output, size_t *output_size, size_t *processed) {
    assert(coder);
    assert(buffer);
    assert(output);

    enum { READ_AHEAD_SIZE = sizeof(uint32_t) };

    /* Measures how much of the input was read */
    *processed = 0;
    size_t output_pos = 0;

    /* current_read_byte + bit_pos = the current position in the input buffer */
    const uint8_t *current_read_byte = buffer;
    uint8_t bit_pos = 0;

    uint32_t working_bits = 0;
    for (uint8_t bytes_read = 0; bytes_read < READ_AHEAD_SIZE; ++bytes_read) {
        /* Populate working_bits with first READ_AHEAD_SIZE bytes in the buffer */
        working_bits <<= 8;
        if (bytes_read < len) {
            working_bits |= *(current_read_byte++);
        }
    }

    while (*processed < len && output_pos < *output_size) {

        uint16_t symbol;
        size_t bits_read = coder->decode(working_bits, &symbol, coder->userdata);
        assert(bits_read > 0);

        /* Update processed based on how many bits we read */
        *processed += (bits_read + bit_pos) / 8;

        if (symbol == coder->eos_symbol) {
            /* Handle EOS */

            *output_size = output_pos;
            if ((bits_read + bit_pos) % 8 != 0) {
                /* If not even number of bits processed, round up for final byte */
                ++*processed;
            }

            if (*processed == len) {
                /* If on the last byte, success */
                return AWS_HUFFMAN_DECODE_EOS_REACHED;
            } else {
                /* If EOS found and more bytes to decode, error */
                return AWS_HUFFMAN_DECODE_ERROR;
            }
        } else {
            assert(symbol < UINT8_MAX);

            /* Store the found symbol */
            output[output_pos++] = (char)symbol;

            /* Read from bytes in the buffer until all read bytes have been replaced */
            while (bits_read > 0) {
                uint8_t bits_in_current = 8 - bit_pos;
                uint8_t bits_from_current = bits_read > bits_in_current ? bits_in_current : bits_read;

                bits_read -= bits_from_current;
                working_bits <<= bits_from_current;
                if (current_read_byte - buffer < len) {
                    /* Read the appropiate number of bits from this byte */
                    working_bits |= (*current_read_byte << bit_pos) >> (8 - bits_from_current);
                }

                bit_pos += bits_from_current;
                if (bit_pos == 8) {
                    ++current_read_byte;
                    bit_pos = 0;
                }
            }
        }
    }

    *output_size = output_pos;
    return AWS_HUFFMAN_DECODE_ERROR;
}
