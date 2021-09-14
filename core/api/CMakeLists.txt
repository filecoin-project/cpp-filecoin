#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(common_api INTERFACE)
target_link_libraries(common_api INTERFACE
        OpenSSL::Crypto
        )

add_subdirectory(full_node)
add_subdirectory(rpc)
add_subdirectory(storage_miner)
