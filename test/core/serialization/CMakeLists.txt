#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(serialization_vectors_test
    serialization_vectors_test.cpp
    )
target_link_libraries(serialization_vectors_test
    rpc
    )
