#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

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

addtest(le_encoder_test
    le_encoder_test.cpp
    )
target_link_libraries(le_encoder_test
    Boost::boost
    blob
    buffer
    )

addtest(outcome_test
    outcome_test.cpp
    )
target_link_libraries(outcome_test
    outcome
    todo_error
    )

addtest(tarutil_test
        tarutil_test.cpp
        )
target_link_libraries(tarutil_test
        tarutil
        base_fs_test
        )
