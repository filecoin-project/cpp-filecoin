#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(toolchain
    impl/address_matcher_v0.cpp
    impl/address_matcher_v2.cpp
    impl/address_matcher_v3.cpp
    impl/toolchain.cpp
    )

target_link_libraries(toolchain
    actor
    )
