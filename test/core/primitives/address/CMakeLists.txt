# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_address_test
    address_test.cpp
    )
target_link_libraries(filecoin_address_test
    filecoin_address
    )

addtest(filecoin_address_codec_test
    address_codec_test.cpp
    )
target_link_libraries(filecoin_address_codec_test
    filecoin_address
    )

addtest(filecoin_address_verifier_test
    address_verifier_test.cpp
    )
target_link_libraries(filecoin_address_verifier_test
    filecoin_address
    filecoin_bls_provider
    )
