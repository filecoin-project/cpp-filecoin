#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(cpp_ledger_test
    apdu_wrapper_test.cpp
    ledger_utils_test.cpp
    ledger_filecoin_test.cpp
    )
target_link_libraries(cpp_ledger_test
    ledger_filecoin
    )
