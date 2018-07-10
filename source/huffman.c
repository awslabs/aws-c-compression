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

#include <aws/common/common.h>
#include <aws/common/byte_buf.h>

#include <assert.h>

#define BITSIZEOF(val) (sizeof(val) * 8)

void aws_huffman_encoder_init(struct aws_huffman_encoder *encoder, struct aws_huffman_symbol_coder *coder) {
    assert(encoder);
    assert(coder);

    AWS_ZERO_STRUCT(*encoder);
    encoder->coder = coder;
    encoder->eos_padding = UINT8_MAX;
}

void aws_huffman_encoder_reset(struct aws_huffman_encoder *encoder) {
    assert(encoder);

    uint8_t eos_padding = encoder->eos_padding;
    aws_huffman_encoder_init(encoder, encoder->coder);
    encoder->eos_padding = eos_padding;
}

void aws_huffman_decoder_init(struct aws_huffman_decoder *decoder, struct aws_huffman_symbol_coder *coder) {
    assert(decoder);
    assert(coder);

    AWS_ZERO_STRUCT(*decoder);
    decoder->coder = coder;
}

void aws_huffman_decoder_reset(struct aws_huffman_decoder *decoder) {
    aws_huffman_decoder_init(decoder, decoder->coder);
}

/* Much of encode is written in a helper function,
   so this struct helps avoid passing all the parameters through by hand */
struct encoder_state {
    struct aws_huffman_encoder *encoder;
    struct aws_byte_cursor output_cursor;
    uint8_t working;
    uint8_t bit_pos;
};

/* Helper function to write a single bit_pattern to memory (or working_bits if out of buffer space) */
static aws_common_error encode_write_bit_pattern(struct encoder_state *state, struct aws_huffman_code bit_pattern) {

    uint8_t bits_to_write = bit_pattern.num_bits;
    while (bits_to_write > 0) {
        uint8_t bits_for_current = bits_to_write > state->bit_pos ? state->bit_pos : bits_to_write;
        /* Chop off the top 0s and bits that have already been read */
        uint8_t bits_to_cut = (BITSIZEOF(bit_pattern.pattern) - bit_pattern.num_bits) + (bit_pattern.num_bits - bits_to_write);

        /* Write the appropiate number of bits to this byte
            Shift to the left to cut any unneeded bits
            Shift to the right to position the bits correctly */
        state->working |= (bit_pattern.pattern << bits_to_cut) >> (32 - state->bit_pos);

        bits_to_write -= bits_for_current;
        state->bit_pos -= bits_for_current;

        if (state->bit_pos == 0) {
            /* Save the whole byte */
            aws_byte_cursor_write_u8(&state->output_cursor, state->working);

            state->bit_pos = 8;
            state->working = 0;

            if (state->output_cursor.len == 0) {
                /* Write all the remaining bits to working_bits and return */

                bits_to_cut += bits_for_current;

                state->encoder->overflow_bits.num_bits = bits_to_write;
                state->encoder->overflow_bits.pattern = (bit_pattern.pattern << bits_to_cut) >> (32 - bits_to_write);

                return AWS_ERROR_SHORT_BUFFER;
            }
        }
    }

    return AWS_ERROR_SUCCESS;
}

#define CHECK_WRITE_BITS(bit_pattern) do {                                              \
        aws_common_error result = encode_write_bit_pattern(&state, bit_pattern); \
        if (result != AWS_ERROR_SUCCESS) {                                            \
            *length -= input_cursor.len;                                                \
            *output_size -= state.output_cursor.len;                                    \
            return result;                                                              \
        }                                                                               \
    } while (0)

aws_common_error aws_huffman_encode(struct aws_huffman_encoder *encoder, const char *to_encode, size_t *length, uint8_t *output, size_t *output_size) {
    assert(encoder);
    assert(encoder->coder);
    assert(to_encode);
    assert(length);
    assert(output);
    assert(output_size);

    if (*output_size == 0) {
        return AWS_ERROR_SHORT_BUFFER;
    }

    struct encoder_state state = {
        .working = 0,
        .bit_pos = 8,
    };
    state.encoder = encoder;
    state.output_cursor = aws_byte_cursor_from_array(output, *output_size);

    /* Counters for how far into the output we currently are */
    struct aws_byte_cursor input_cursor = aws_byte_cursor_from_array(to_encode, *length);

    /* Write any bits leftover from previous invocation */
    CHECK_WRITE_BITS(encoder->overflow_bits);
    AWS_ZERO_STRUCT(encoder->overflow_bits);

    while (input_cursor.len) {
        uint8_t new_byte = 0;
        aws_byte_cursor_read_u8(&input_cursor, &new_byte);
        struct aws_huffman_code code_point = encoder->coder->encode(new_byte, encoder->coder->userdata);

        CHECK_WRITE_BITS(code_point);
    }

    /* The following code only runs when the buffer has written successfully */

    /* If whole buffer processed, write EOS */
    struct aws_huffman_code eos_cp;
    eos_cp.pattern = encoder->eos_padding;
    eos_cp.num_bits = state.bit_pos;
    encode_write_bit_pattern(&state, eos_cp);
    assert(state.bit_pos == 8);

    *length -= input_cursor.len;
    *output_size -= state.output_cursor.len;
    return AWS_ERROR_SUCCESS;
}

#undef CHECK_WRITE_BITS

/* Decode's reading is written in a helper function,
   so this struct helps avoid passing all the parameters through by hand */
struct decoder_state {
    struct aws_huffman_decoder *decoder;
    struct aws_byte_cursor input_cursor;
};

static void decode_fill_working_bits(struct decoder_state *state) {
    /* Read from bytes in the buffer until there are enough bytes to process */
    while (state->decoder->num_bits < 32 && state->input_cursor.len) {
        /* Read the appropiate number of bits from this byte */
        uint8_t new_byte = 0;
        aws_byte_cursor_read_u8(&state->input_cursor, &new_byte);

        uint64_t positioned = ((uint64_t)new_byte) << (BITSIZEOF(state->decoder->working_bits) - 8 - state->decoder->num_bits);
        state->decoder->working_bits |= positioned;

        state->decoder->num_bits += 8;
    }
}

aws_common_error aws_huffman_decode(struct aws_huffman_decoder *decoder, const uint8_t *to_decode, size_t *length, char *output, size_t *output_size) {
    assert(decoder);
    assert(decoder->coder);
    assert(to_decode);
    assert(length);
    assert(output);
    assert(output_size);

    if (*output_size == 0) {
        return AWS_ERROR_SHORT_BUFFER;
    }

    struct decoder_state state;
    state.decoder = decoder;
    state.input_cursor = aws_byte_cursor_from_array(to_decode, *length);

    /* Measures how much of the input was read */
    size_t bits_left = decoder->num_bits + *length * 8;
    struct aws_byte_cursor output_cursor = aws_byte_cursor_from_array(output, *output_size);

    while (1) {

        decode_fill_working_bits(&state);

        uint16_t symbol;
        uint8_t bits_read = decoder->coder->decode(decoder->working_bits >> 32, &symbol, decoder->coder->userdata);

        if (bits_read == 0 || bits_read >= bits_left) {
            /* Check if the buffer has been overrun.
               Note: because of the check in decode_fill_working_bits,
               the buffer won't actually overrun, instead there will
               be 0's in the bottom of working_bits. */

            *length -= state.input_cursor.len;
            *output_size -= output_cursor.len;

            return AWS_ERROR_SUCCESS;
        }

        if (output_cursor.len == 0) {
            /* Check if we've hit the end of the output buffer */
            *length -= state.input_cursor.len;
            return AWS_ERROR_SHORT_BUFFER;
        }

        assert(symbol < UINT8_MAX);

        bits_left -= bits_read;
        decoder->working_bits <<= bits_read;
        decoder->num_bits -= bits_read;

        /* Store the found symbol */
        aws_byte_cursor_write_u8(&output_cursor, (uint8_t)symbol);
    }

    /* This case is unreachable */
    assert(0);
}
