#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(go_test
    math_test.cpp
    )
target_link_libraries(go_test
    Boost::boost
    )
