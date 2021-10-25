#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(toolchain
    impl/toolchain.cpp
    impl/common_address_matcher.cpp
    )

target_link_libraries(toolchain
    actor
    )
