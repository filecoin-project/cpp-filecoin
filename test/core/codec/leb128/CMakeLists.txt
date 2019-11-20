#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(leb128_test
        leb128_test.cpp
    )
target_link_libraries(leb128_test
        leb128
        )
