#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(blake2_test
    blake2_test.cpp
    )
target_link_libraries(blake2_test
    blake2
    hexutil
    )

addtest(bls_provider_test
    bls_provider_test.cpp
    )
target_link_libraries(bls_provider_test
    bls_provider
    )

addtest(murmur_test
    murmur_test.cpp
    )
target_link_libraries(murmur_test
    murmur
    )

add_subdirectory(randomness)
add_subdirectory(vrf)

