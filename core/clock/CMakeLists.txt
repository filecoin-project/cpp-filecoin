#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_clock
    chain_epoch_clock.cpp
    impl/chain_epoch_clock_impl.cpp
    impl/utc_clock_impl.cpp
    time.cpp
    )
target_compile_definitions(filecoin_clock PRIVATE
    BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
    )
target_link_libraries(filecoin_clock
    Boost::date_time
    filecoin_outcome
    p2p::p2p_uvarint
    )
