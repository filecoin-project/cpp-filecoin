#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(builtin)

add_library(actor
    actor.cpp
    impl/invoker_impl.cpp
    builtin/payment_channel/payment_channel_actor.cpp builtin/payment_channel/payment_channel_actor.hpp)
target_link_libraries(actor
    account_actor
    address
    buffer
    cid
    exit_code
    init_actor
    multisig_actor
    storage_power_actor
    cron_actor
    )
