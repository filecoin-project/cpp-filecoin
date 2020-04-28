#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(fuhon_fsm fsm.cpp error.cpp)
target_link_libraries(fuhon_fsm
    p2p::p2p
    p2p::asio_scheduler
    outcome
    Boost::boost)
