#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api
    make.cpp
    )
target_link_libraries(api
    address
    chain_store
    cid
    )

add_library(rpc
    rpc/make.cpp
    rpc/json_errors.cpp
    )
target_link_libraries(rpc
    api

    # TODO: remove
    filecoin_hasher
    )
