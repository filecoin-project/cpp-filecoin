#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(address_test
    address_test.cpp
    )
target_link_libraries(address_test
    address
    )
