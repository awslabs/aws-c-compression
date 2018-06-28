#ifndef AWS_COMPRESSION_PRIVATE_HUFFMAN_STATIC_DECODE_H
#define AWS_COMPRESSION_PRIVATE_HUFFMAN_STATIC_DECODE_H

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

#ifdef __cplusplus
extern "C" {
#endif

struct aws_huffman_coder *hpack_get_coder();

#ifdef __cplusplus
}
#endif

#endif // AWS_COMPRESSION_PRIVATE_HUFFMAN_STATIC_DECODE_H
