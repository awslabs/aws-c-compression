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

#include <assert.h>

void aws_huffman_encoder_init(struct aws_huffman_encoder *encoder, struct aws_huffman_character_coder *coder) {
    assert(encoder);
    assert(coder);

    AWS_ZERO_STRUCT(*encoder);
    encoder->coder = coder;
}

void aws_huffman_decoder_init(struct aws_huffman_decoder *decoder, struct aws_huffman_character_coder *coder) {
    assert(decoder);
    assert(coder);

    AWS_ZERO_STRUCT(*decoder);
    decoder->coder = coder;
}

/* Much of encode is written in a helper function,
   so this struct helps avoid passing all the parameters through by hand */
struct encoder_state {
    struct aws_huffman_encoder *encoder;
    uint8_t *output;
    size_t output_size;
    size_t output_pos;
    uint8_t working;
    uint8_t bit_pos;
};

/* Helper function to write a single bit_pattern to memory (or working_bits if out of buffer space) */
static aws_huffman_coder_state encode_write_bit_pattern(struct encoder_state *state, struct aws_huffman_bit_pattern bit_pattern) {

    uint8_t bits_to_write = bit_pattern.num_bits;
    while (bits_to_write > 0) {
        uint8_t bits_for_current = bits_to_write > state->bit_pos ? state->bit_pos : bits_to_write;
        /* Chop off the top 0s and bits that have already been read */
        uint8_t bits_to_cut = (32 - bit_pattern.num_bits) + (bit_pattern.num_bits - bits_to_write);

        /* Write the appropiate number of bits to this byte
            Shift to the left to cut any unneeded bits
            Shift to the right to position the bits coorectly */
        state->working |= (bit_pattern.pattern << bits_to_cut) >> (32 - state->bit_pos);

        bits_to_write -= bits_for_current;
        state->bit_pos -= bits_for_current;

        if (state->bit_pos == 0) {
            /* Save the whole byte */
            state->output[state->output_pos++] = state->working;

            if (state->output_pos == state->output_size) {
                /* Write all the remaining bits to working_bits and return */

                bits_to_cut += bits_for_current;

                state->encoder->working_bits.num_bits = bits_to_write;
                state->encoder->working_bits.pattern = (bit_pattern.pattern << bits_to_cut) >> (32 - bits_to_write);

                return AWS_HUFFMAN_DECODE_NEED_MORE_OUTPUT;
            }

            state->bit_pos = 8;
            state->working = 0;
        }
    }

    return AWS_HUFFMAN_DECODE_EOS_REACHED;
}

aws_huffman_coder_state aws_huffman_encode(struct aws_huffman_encoder *encoder, const char *to_encode, size_t length, uint8_t *output, size_t *output_size, size_t *processed) {
    assert(encoder);
    assert(encoder->coder);
    assert(to_encode);
    assert(output);

    struct encoder_state state = {
        .encoder = encoder,
        .output = output,
        .output_size = *output_size,
        .output_pos = 0,
        .working = 0,
        .bit_pos = 8,
    };

    /* Counters for how far into the output we currently are */
    *processed = 0;

#define CHECK_WRITE_BITS(bit_pattern) do {                                              \
        aws_huffman_coder_state result = encode_write_bit_pattern(&state, bit_pattern);  \
        if (result != AWS_HUFFMAN_DECODE_EOS_REACHED) { return result; }                \
    } while (0)

    /* Write any bits leftover from previous invocation */
    CHECK_WRITE_BITS(encoder->working_bits);
    AWS_ZERO_STRUCT(encoder->working_bits);

    while (*processed < length) {
        /* Write the actual data passed */
        struct aws_huffman_bit_pattern code_point = encoder->coder->encode(
            to_encode[(*processed)++], encoder->coder->userdata);

        CHECK_WRITE_BITS(code_point);
    }

    /* The following code only runs when the buffer has written successfully */

    if (!state.encoder->eos_written) {
        /* If whole buffer processed, wrote EOS */
        struct aws_huffman_bit_pattern eos_cp = encoder->coder->encode(
            encoder->coder->eos_symbol, encoder->coder->userdata);

        /* Signal the EOS was already written to working_bits, don't try again
           This is important if the output buffer fills up while writting EOS */
        state.encoder->eos_written = 1;
        CHECK_WRITE_BITS(eos_cp);
    }

#undef CHECK_WRITE_BITS

    if (state.bit_pos < 8) {
        /* Pad the rest out with 1s */
        uint8_t padding_mask = UINT8_MAX >> (8 - state.bit_pos);
        state.working |= padding_mask;
        output[state.output_pos++] = state.working;
    }

    state.encoder->eos_written = 0;

    *output_size = state.output_pos;
    return AWS_HUFFMAN_DECODE_EOS_REACHED;
}

/* Decode's reading is written in a helper function,
   so this struct helps avoid passing all the parameters through by hand */
struct decoder_state {
    struct aws_huffman_decoder *decoder;
    const uint8_t *current_read_byte;
    const uint8_t *buffer_end;
    size_t working;
};

static void decode_fill_working_bits(struct decoder_state *state, uint8_t bits_to_read) {
    /* Read from bytes in the buffer until all bytes have been replaced */
    while (bits_to_read > 0) {
        uint8_t bits_in_current = 8 - state->decoder->bit_pos;
        uint8_t bits_from_current = bits_to_read > bits_in_current ? bits_in_current : bits_to_read;

        bits_to_read -= bits_from_current;
        state->working <<= bits_from_current;
        if (state->current_read_byte < state->buffer_end) {
            /* Read the appropiate number of bits from this byte */
            state->working |= (*state->current_read_byte << state->decoder->bit_pos) >> (8 - bits_from_current);
        }

        state->decoder->bit_pos += bits_from_current;
        if (state->decoder->bit_pos == 8) {
            state->decoder->bit_pos = 0;
            ++state->current_read_byte;
        }
    }
}

aws_huffman_coder_state aws_huffman_decode(struct aws_huffman_decoder *decoder, const uint8_t *to_decode, size_t length, char *output, size_t *output_size, size_t *processed) {
    assert(decoder);
    assert(decoder->coder);
    assert(to_decode);
    assert(output);

    enum { READ_AHEAD_SIZE = sizeof(uint32_t) };

    struct decoder_state state = {
        .decoder = decoder,
        .current_read_byte = to_decode,
        .buffer_end = to_decode + length,
        .working = 0,
    };

    /* Measures how much of the input was read */
    *processed = 0;
    size_t output_pos = 0;

    decode_fill_working_bits(&state, 32);

    while (1) {

        if (output_pos == *output_size) {
            /* Check if we've hit the end of the output buffer */
            return AWS_HUFFMAN_DECODE_NEED_MORE_OUTPUT;
        }

        uint16_t symbol;
        size_t bits_read = decoder->coder->decode(state.working, &symbol, decoder->coder->userdata);
        assert(bits_read > 0);

        if (*processed * 8 + decoder->bit_pos + bits_read > length * 8) {
            /* Check if the buffer has been overrun.
               Note: because of the check in decode_fill_working_bits,
               the buffer won't actually overrun, instead there will
               be 0's in the bottom of working_bits. */
            *output_size = output_pos;
            return AWS_HUFFMAN_DECODE_NEED_MORE_DATA;
        }

        if (symbol == decoder->coder->eos_symbol) {
            /* Handle EOS */

            *output_size = output_pos;

            /* Update processed in include last partial byte read
               Note: 1 + ((x - 1) / y) is a way to round x/y up */
            *processed += 1 + ((bits_read + decoder->bit_pos - 1) / 8);

            /* Reset the state */
            decoder->bit_pos = 0;

            if (*processed == length) {
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

            /* Update processed to include any bytes that have been completely processed
               DOES NOT include partial bytes read */
            *processed += (bits_read + decoder->bit_pos) / 8;

            decode_fill_working_bits(&state, bits_read);
        }
    }

    /* This case is unreachable */
    assert(0);
}