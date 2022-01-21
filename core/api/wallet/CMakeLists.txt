#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(wallet
    local_wallet.cpp
    ledger_wallet.cpp
    ledger.cpp
    )
target_link_libraries(wallet
    address
    ledger_filecoin
    )
