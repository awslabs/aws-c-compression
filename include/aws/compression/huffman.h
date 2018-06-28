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

struct aws_huffman_bit_pattern {
    uint32_t pattern;
    uint8_t num_bits;
};

typedef struct aws_huffman_bit_pattern (*aws_huffman_character_encoder)(uint16_t symbol, void *userdata);
typedef size_t (*aws_huffman_character_decoder)(uint32_t bit_pattern, uint16_t *symbol, void *userdata);

struct aws_huffman_coder {
    aws_huffman_character_encoder encode;
    aws_huffman_character_decoder decode;
    uint16_t eos_symbol;
    void *userdata;
};

typedef enum aws_huffman_decoder_state {
    AWS_HUFFMAN_DECODE_NEED_MORE,
    AWS_HUFFMAN_DECODE_EOS_REACHED,
    AWS_HUFFMAN_DECODE_ERROR
} aws_huffman_decoder_state;

#ifdef __cplusplus
extern "C" {
#endif

size_t aws_huffman_encode(struct aws_huffman_coder *coder, const char *to_encode, size_t length, uint8_t *output);
aws_huffman_decoder_state aws_huffman_decode(struct aws_huffman_coder *coder, const uint8_t *buffer, size_t len, char *output, size_t *output_size, size_t *processed);

#ifdef __cplusplus
}
#endif

#endif // AWS_COMPRESSION_HUFFMAN_H
