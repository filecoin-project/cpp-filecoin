#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(time_test
    time_test.cpp
    )
target_link_libraries(time_test
    clock
    )

addtest(chain_epoch_clock_test
    chain_epoch_clock_test.cpp
    )
target_link_libraries(chain_epoch_clock_test
    clock
    )
