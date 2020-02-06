# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(json_codec
    json_decode_stream.cpp
    json_encode_stream.cpp
    )
target_link_libraries(json_codec
    RapidJSON::rapidjson
    )
