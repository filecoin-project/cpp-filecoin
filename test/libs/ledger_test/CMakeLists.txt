#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(ledger_test
    ledger_test.cpp
    )
target_link_libraries(ledger_test
    wallet
    in_memory_storage
    cbor
    )
