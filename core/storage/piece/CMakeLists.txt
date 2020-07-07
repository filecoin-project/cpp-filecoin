#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(piece_storage
    impl/piece_storage_impl.cpp
    impl/piece_storage_error.cpp
    )
target_link_libraries(piece_storage
    buffer
    cbor
    outcome
    )
