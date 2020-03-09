#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_exit_code_test
    exit_code_test.cpp
    )
target_link_libraries(filecoin_exit_code_test
    filecoin_exit_code
    )
