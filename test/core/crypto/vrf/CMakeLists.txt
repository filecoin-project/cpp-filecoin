# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(vrfhash_test
    vrfhash_test.cpp
    )
target_link_libraries(vrfhash_test
    vrf_provider
    blob
    buffer
    )

addtest(vrf_provider_test
    vrf_provider_test.cpp
    )
target_link_libraries(vrf_provider_test
    vrf_provider
    )
