# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_vrf_encoder_test
    vrf_encoder_test.cpp
    )
target_link_libraries(filecoin_vrf_encoder_test
    filecoin_vrf_provider
    )

addtest(filecoin_vrf_provider_test
    vrf_provider_test.cpp
    )
target_link_libraries(filecoin_vrf_provider_test
    filecoin_vrf_provider
    filecoin_bls_provider
    )
