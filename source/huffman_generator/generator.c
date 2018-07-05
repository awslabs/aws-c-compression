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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct bit_pattern {
    uint8_t num_bits;
    uint32_t bits;
};

struct code_point {
    uint16_t symbol;
    struct bit_pattern pattern;
};

static const size_t num_code_points = 257;
static struct code_point code_points[num_code_points];

static void skip_whitespace(const char **str) {
    while (**str == ' ' || **str == '\t') {
        ++(*str);
    }
}

static void read_past_comma(const char **str) {
    while (**str != ',') {
        ++(*str);
    }
    ++(*str);
}

void read_code_points(const char *input_path) {

    memset(code_points, 0, sizeof(code_points));
    FILE *file = fopen(input_path, "r");

    static const char HC_KEYWORD[] = "HUFFMAN_CODE";
    static const size_t HC_KW_LEN = sizeof(HC_KEYWORD) - 1;

    int is_comment = 0;
    size_t code_index = 0;
    char line[120];
    while (fgets(line, sizeof(line), file) != NULL) {
        const size_t line_length = strlen(line);

        if (line[0] == '#') {
            /* Ignore preprocessor directives */
            continue;
        } else {
            /* Check for comments */
            for (size_t i = 0; i < line_length - 1; ++i) {
                if (!is_comment) {
                    if (line[i] == '/' && line[i + 1] == '*') {
                        is_comment = 1;
                    } else if (strncmp(&line[i], HC_KEYWORD, HC_KW_LEN) == 0) {
                        /* Found code, parse it */
                        struct code_point *code = &code_points[code_index++];

                        /* Skip macro */
                        const char *current_char = &line[i + HC_KW_LEN];
                        skip_whitespace(&current_char);
                        /* Skip ( */
                        assert(*current_char == '(');
                        ++current_char;

                        /* Parse symbol */
                        code->symbol = (uint16_t)atoi(current_char);

                        read_past_comma(&current_char);
                        /* Skip the binary string form */
                        read_past_comma(&current_char);

                        /* Parse bits */
                        code->pattern.bits = (uint32_t)strtol(current_char, NULL, 16);

                        read_past_comma(&current_char);

                        code->pattern.num_bits = (uint32_t)atoi(current_char);
                    }
                } else if (line[i] == '*' && line[i + 1] == '/') {
                    is_comment = 0;
                }
            }
        }
    }

    fclose(file);

    assert(code_index == 257);
}

void bit_pattern_write(struct bit_pattern *pattern, FILE* file) {
    for (int bit_idx = pattern->num_bits - 1; bit_idx >= 0; --bit_idx) {
        char bit = ((pattern->bits >> bit_idx) & 0x1) ? '1' : '0';
        fprintf(file, "%c", bit);
    }
}

struct huffman_node {
    struct code_point *value;

    struct bit_pattern pattern;
    struct huffman_node *children[2];
};

struct huffman_node *huffman_node_new(struct bit_pattern pattern) {

    struct huffman_node *node = (struct huffman_node *)malloc(sizeof(struct huffman_node));
    memset(node, 0, sizeof(struct huffman_node));
    node->pattern = pattern;
    return node;
}

struct huffman_node *huffman_node_new_value(struct code_point* value) {

    struct huffman_node *node = (struct huffman_node *)malloc(sizeof(struct huffman_node));
    memset(node, 0, sizeof(struct huffman_node));
    node->value = value;
    node->pattern = value->pattern;
    return node;
}

/* note: Does not actually free the memory.
   This is useful so this function may be called on the tree root. */
void huffman_node_clean_up(struct huffman_node *node) {

    for (int i = 0; i < 2; ++i) {
        if (node->children[i]) {
            huffman_node_clean_up(node->children[i]);
            free(node->children[i]);
        }
    }

    memset(node, 0, sizeof(struct huffman_node));
}

/* This function writes what to do if the pattern for node is a match */
void huffman_node_write_decode_handle_value(struct huffman_node *node, FILE *file) {
    if (node->value) {
        /* Attempt to inline value return */
        fprintf(file,
"        *symbol = %u;\n"
"        return %u;\n",
            node->value->symbol,
            node->value->pattern.num_bits);
    } else {
        /* Otherwise go to branch check */
        fprintf(file,
"        goto node_");
        bit_pattern_write(&node->pattern, file);
        fprintf(file, ";\n");
    }
}

void huffman_node_write_decode(struct huffman_node *node, FILE *file, uint8_t current_bit) {

    /* Value nodes should have been inlined into parent branch checks */
    assert(!node->value);
    assert(node->children[0] && node->children[1]);

    static int write_label = 0;

    if (write_label) {
        /* Write this node's label after the first run */
        fprintf(file,
"node_");
        bit_pattern_write(&node->pattern, file);
        fprintf(file, ":\n");
    }

    write_label = 1;

    /* Check 1 bit pattern */
    struct huffman_node *child_1 = node->children[1];

    uint32_t single_bit_mask = 1 << (31 - current_bit);
    uint32_t left_aligned_pattern = child_1->pattern.bits << (32 - child_1->pattern.num_bits);
    uint32_t check_pattern = left_aligned_pattern & single_bit_mask;
    fprintf(file,
"    if (bit_pattern & 0x%x) {\n", check_pattern);

    huffman_node_write_decode_handle_value(child_1, file);

    fprintf(file,
"    } else {\n");

    /* Child 0 is valid, go there */
    huffman_node_write_decode_handle_value(node->children[0], file);

    fprintf(file,
"    }\n\n");

    /* Recursively write child nodes */
    for (uint8_t i = 0; i < 2; ++i) {
        struct huffman_node *child = node->children[i];
        if (child && !child->value) {
            huffman_node_write_decode(child, file, current_bit + 1);
        }
    }
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        fprintf(stderr,
            "generator expects 3 arguments: [input file] [output file] [encoding name]\n"
            "A function of the following signature will be exported:\n"
            "struct aws_huffman_character_coder *[encoding_get_coder()");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    const char *decoder_name = argv[3];

    read_code_points(input_file);

    struct huffman_node tree_root;
    memset(&tree_root, 0, sizeof(struct huffman_node));

    /* Populate the tree */
    for (size_t code_point_idx = 0; code_point_idx < num_code_points; ++code_point_idx) {

        struct code_point *value = &code_points[code_point_idx];
        struct huffman_node *current = &tree_root;

        for (int bit_idx = value->pattern.num_bits - 1; bit_idx >= 0; --bit_idx) {

            struct bit_pattern pattern = value->pattern;
            pattern.bits >>= bit_idx;
            pattern.num_bits = value->pattern.num_bits - bit_idx;

            uint8_t encoded_bit = (uint8_t)((pattern.bits) & 0x01);
            assert(encoded_bit == 0 || encoded_bit == 1);

            if (current->children[encoded_bit]) {
                /* Not at the end yet, keep traversing */
                current = current->children[encoded_bit];
            } else if (bit_idx > 0) {
                assert(!current->children[encoded_bit]);
                /* Not at the end yet, but this is the first time down this path. */
                struct huffman_node *new_node = huffman_node_new(pattern);
                current->children[encoded_bit] = new_node;
                current = new_node;
            } else {
                assert(!current->children[encoded_bit]);
                /* Done traversing, add value as leaf */
                current->children[encoded_bit] = huffman_node_new_value(value);
                break;
            }
        }
    }

    /* Open the file */
    FILE *file = fopen(output_file, "w");
    assert(file);

    /* Write the file/function header */
    fprintf(file,
"/*\n"
"* Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.\n"
"*\n"
"* Licensed under the Apache License, Version 2.0 (the \"License\").\n"
"* You may not use this file except in compliance with the License.\n"
"* A copy of the License is located at\n"
"*\n"
"*  http://aws.amazon.com/apache2.0\n"
"*\n"
"* or in the \"license\" file accompanying this file. This file is distributed\n"
"* on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either\n"
"* express or implied. See the License for the specific language governing\n"
"* permissions and limitations under the License.\n"
"*/\n"
"\n"
"/* WARNING: THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT. */\n"
"\n"
"#include <aws/compression/huffman.h>\n"
"\n"
"#include <assert.h>\n"
"\n"
"static struct aws_huffman_bit_pattern code_points[] = {\n");

    for (size_t code_point_idx = 0; code_point_idx < num_code_points; ++code_point_idx) {
        struct code_point *cp = &code_points[code_point_idx];
        fprintf(file,
"    { .pattern = 0x%x, .num_bits = %u }, /* %u */\n",
            cp->pattern.bits, cp->pattern.num_bits, cp->symbol);
    }

    fprintf(file,
"};\n"
"\n"
"struct aws_huffman_bit_pattern encode_character(uint16_t symbol, void *userdata) {\n"
"    (void)userdata;\n\n"
"    assert(symbol < %zu);\n"
"    return code_points[symbol];\n"
"}\n"
"\n"
"static size_t decode_character(uint32_t bit_pattern, uint16_t *symbol, void *userdata) {\n"
"    (void)userdata;\n\n", num_code_points);

     /* Traverse the tree */
    huffman_node_write_decode(&tree_root, file, 0);

    /* Write the function footer & encode header */
    fprintf(file,
"}\n"
"\n"
"struct aws_huffman_character_coder *%s_get_coder() {\n"
"\n"
"    static struct aws_huffman_character_coder coder = {\n"
"        .encode = encode_character,\n"
"        .decode = decode_character,\n"
"        .eos_symbol = %zu,\n"
"        .userdata = NULL,\n"
"    };\n"
"    return &coder;\n"
"}\n", decoder_name, num_code_points - 1);

    fclose(file);

    huffman_node_clean_up(&tree_root);

    return 0;
}