# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(tipset_test
    tipset_test.cpp
    )
target_link_libraries(tipset_test
    tipset
    )

addtest(tipset_key_test
    tipset_key_test.cpp
    )
target_link_libraries(tipset_key_test
    tipset
    )
