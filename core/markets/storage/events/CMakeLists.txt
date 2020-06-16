#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(events
    impl/events_impl.cpp
    )
target_link_libraries(events
    api
    p2p::asio_scheduler
    outcome
    )
