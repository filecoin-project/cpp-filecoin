#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(stored_ask_test
    stored_ask_test.cpp
    )
target_link_libraries(stored_ask_test
    fuhon_stored_ask
    secp256k1_provider
    bls_provider
    in_memory_storage
    miner_types
    )
