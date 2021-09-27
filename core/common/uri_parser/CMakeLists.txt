# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(uri_parser
    uri_parser.cpp
    percent_encoding.cpp
    )
# any library is linked to include GSL
target_link_libraries(uri_parser
    outcome
    )
