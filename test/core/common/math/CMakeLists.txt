#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(math_test
    math_test.cpp
    )
target_link_libraries(math_test
    Boost::filesystem
    )
