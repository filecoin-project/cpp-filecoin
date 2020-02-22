# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_tipset_test
    tipset_test.cpp
    )
target_link_libraries(filecoin_tipset_test
    filecoin_tipset
    )

addtest(filecoin_tipset_key_test
    tipset_key_test.cpp
    )
target_link_libraries(filecoin_tipset_key_test
    filecoin_tipset
    )
