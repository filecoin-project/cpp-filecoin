#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(clock
    chain_epoch_clock.cpp
    impl/chain_epoch_clock_impl.cpp
    impl/utc_clock_impl.cpp
    time.cpp
    )
target_link_libraries(clock
    Boost::date_time
    outcome
    )
