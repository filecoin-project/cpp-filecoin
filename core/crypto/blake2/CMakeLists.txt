#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blake2
    blake2b160.cpp
    )
target_link_libraries(blake2
    OpenSSL::Crypto
    blob
    outcome
    )
