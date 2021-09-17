#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(jwt INTERFACE)

target_link_libraries(jwt INTERFACE
        OpenSSL::Crypto
        )
