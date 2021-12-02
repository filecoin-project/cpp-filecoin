# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(paych_maker_test
    maker_test.cpp
    )
target_link_libraries(paych_maker_test
    in_memory_storage
    paych_maker
    )

addtest(paych_vouchers_test
    vouchers_test.cpp
    )
target_link_libraries(paych_vouchers_test
    in_memory_storage
    ipfs_datastore_in_memory
    paych_vouchers
    )
