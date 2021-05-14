# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(tipset_test
    tipset_test.cpp
    )
target_link_libraries(tipset_test
    tipset
    )

addtest(load_test
        load_test.cpp
        )
target_link_libraries(load_test
        tipset
        ipfs_datastore_in_memory
        )
