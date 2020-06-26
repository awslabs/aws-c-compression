/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <assert.h> /**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */\n"
        "\n"
        "/* WARNING: THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT. */\n"
        "/* clang-format off */\n"
        "\n"
        "#include <aws/compression/huffman.h>\n"
        "\n"
        "static struct aws_huffman_code code_points[] = {\n");

    for (size_t i = 0; i < num_code_points; ++i) {
        struct huffman_code_point *cp = &code_points[i];
        fprintf(
            file,
            "    { .pattern = 0x%x, .num_bits = %u }, /* '%c' %u */\n",
            cp->code.bits,
            cp->code.num_bits,
            isprint(cp->symbol) ? cp->symbol : ' ',
            cp->symbol);
    }

    fprintf(
        file,
        "};\n"
        "\n"
        "static struct aws_huffman_code encode_symbol(uint8_t symbol, void "
        "*userdata) {\n"
        "    (void)userdata;\n\n"
        "    return code_points[symbol];\n"
        "}\n"
        "\n"
        "/* NOLINTNEXTLINE(readability-function-size) */\n"
        "static uint8_t decode_symbol(uint32_t bits, uint8_t *symbol, void "
        "*userdata) {\n"
        "    (void)userdata;\n\n");

    /* Traverse the tree */
    huffman_node_write_decode(&tree_root, file, 0);

    /* Write the function footer & encode header */
    fprintf(
        file,
        "}\n"
        "\n"
        "struct aws_huffman_symbol_coder *%s_get_coder(void) {\n"
        "\n"
        "    static struct aws_huffman_symbol_coder coder = {\n"
        "        .encode = encode_symbol,\n"
        "        .decode = decode_symbol,\n"
        "        .userdata = NULL,\n"
        "    };\n"
        "    return &coder;\n"
        "}\n",
        decoder_name);

    fclose(file);

    huffman_node_clean_up(&tree_root);

    return 0;
}
