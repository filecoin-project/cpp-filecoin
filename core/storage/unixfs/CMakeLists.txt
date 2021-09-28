# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(unixfs
    unixfs.cpp
    )
target_link_libraries(unixfs
    cbor
    filecoin_hasher
    ipld_traverser
    )
