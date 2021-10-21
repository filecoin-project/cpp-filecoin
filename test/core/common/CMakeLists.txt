#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(math)

addtest(async_test
    async_test.cpp
    )
target_link_libraries(async_test
    outcome
    )

addtest(bitsutil_test
    bitsutil_test.cpp
    )

addtest(hexutil_test
    hexutil_test.cpp
    )
target_link_libraries(hexutil_test
    hexutil
    )

addtest(blob_test
    blob_test.cpp
    )
target_link_libraries(blob_test
    blob
    )

addtest(outcome_test
    outcome_test.cpp
    )
target_link_libraries(outcome_test
    outcome
    )

addtest(tarutil_test
    tarutil_test.cpp
    )
target_link_libraries(tarutil_test
    tarutil
    base_fs_test
    file
    )
