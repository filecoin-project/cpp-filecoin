#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ledger_filecoin
    impl/ledger_filecoin_manager.cpp
    )
target_link_libraries(ledger_filecoin
    ledger_device
    )
