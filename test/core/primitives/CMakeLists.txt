#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(address_test
    address_test.cpp
    )
target_link_libraries(address_test
    address
    bls_provider
    p2p::p2p_secp256k1_provider
    )

addtest(address_codec_test
    address_codec_test.cpp
    )
target_link_libraries(address_codec_test
    address
    )
