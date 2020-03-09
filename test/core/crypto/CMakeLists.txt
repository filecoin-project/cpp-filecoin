#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_blake2_test
    blake2_test.cpp
    )
target_link_libraries(filecoin_blake2_test
    filecoin_blake2
    filecoin_hexutil
    )

addtest(filecoin_bls_provider_test
    bls_provider_test.cpp
    )
target_link_libraries(filecoin_bls_provider_test
    filecoin_bls_provider
    )

addtest(filecoin_murmur_test
    murmur_test.cpp
    )
target_link_libraries(filecoin_murmur_test
    filecoin_murmur
    )

add_subdirectory(randomness)
add_subdirectory(vrf)

