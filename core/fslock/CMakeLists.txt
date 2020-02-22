#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_fslock
        fslock.cpp
        fslock_error.cpp
        )

target_link_libraries(filecoin_fslock
        filecoin_outcome
        )
