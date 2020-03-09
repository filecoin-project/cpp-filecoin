# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_randomness_provider_test
    randomness_provider_test.cpp
    )
target_link_libraries(filecoin_randomness_provider_test
    filecoin_randomness_provider
    )
