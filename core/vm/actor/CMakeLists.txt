#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(builtin)

add_library(filecoin_actor
    actor.cpp
    impl/invoker_impl.cpp
    )
target_link_libraries(filecoin_actor
    filecoin_account_actor
    filecoin_address
    filecoin_buffer
    filecoin_cid
    filecoin_exit_code
    filecoin_init_actor
    filecoin_miner_actor
    filecoin_multisig_actor
    filecoin_storage_power_actor
    filecoin_cron_actor
    )
