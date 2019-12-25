#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(exit_code_test
    exit_code_test.cpp
    )
target_link_libraries(exit_code_test
    exit_code
    )
