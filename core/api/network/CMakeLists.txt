#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#


add_library(network
        network_api.cpp
        )

target_link_libraries(target_link_libraries(storage_miner_api
        p2p::p2p
        remote_worker
        jwt
        ))