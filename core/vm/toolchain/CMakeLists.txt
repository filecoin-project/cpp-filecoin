#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(toolchain
    impl/toolchain.cpp
    impl/common_address_matcher.cpp
    impl/address_matcher_v0.cpp
    impl/address_matcher_v2.cpp
    impl/address_matcher_v3.cpp
    impl/address_matcher_v4.cpp
    impl/address_matcher_v5.cpp
    )

target_link_libraries(toolchain
    actor
    )
