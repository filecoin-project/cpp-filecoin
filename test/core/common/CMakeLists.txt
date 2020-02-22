#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_hexutil_test
    hexutil_test.cpp
    )
target_link_libraries(filecoin_hexutil_test
    filecoin_hexutil
    )

addtest(filecoin_blob_test
    blob_test.cpp
    )
target_link_libraries(filecoin_blob_test
    filecoin_blob
    )

addtest(filecoin_le_encoder_test
    le_encoder_test.cpp
    )
target_link_libraries(filecoin_le_encoder_test
    Boost::boost
    filecoin_blob
    filecoin_buffer
    )
