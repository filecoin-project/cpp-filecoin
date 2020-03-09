#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_time_test
    time_test.cpp
    )
target_link_libraries(filecoin_time_test
    filecoin_clock
    )

addtest(filecoin_chain_epoch_clock_test
    chain_epoch_clock_test.cpp
    )
target_link_libraries(filecoin_chain_epoch_clock_test
    filecoin_clock
    )
