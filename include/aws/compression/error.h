#ifndef AWS_COMPRESSION_ERROR_H

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

#define AWS_COMPRESSION_ERROR_H

#include <aws/compression/compression.h>

enum aws_compression_error {
    AWS_ERROR_COMPRESSION_UNKNOWN_SYMBOL = AWS_ERROR_ENUM_BEGIN_RANGE(AWS_C_COMPRESSION_PACKAGE_ID),

    AWS_ERROR_END_COMPRESSION_RANGE = AWS_ERROR_ENUM_END_RANGE(AWS_C_COMPRESSION_PACKAGE_ID)
};

#endif /* AWS_COMPRESSION_ERROR_H */
