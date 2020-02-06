#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(builtin)

addtest(actor_test
    actor_test.cpp
    )
target_link_libraries(actor_test
    actor
    hexutil
    )

addtest(init_actor_test
    init_actor_test.cpp
    )
target_link_libraries(init_actor_test
    actor
    hexutil
    ipfs_datastore_in_memory
    runtime
    state_tree
    )

addtest(cron_actor_test
    cron_actor_test.cpp
    )
target_link_libraries(cron_actor_test
    actor
    hexutil
    )

addtest(invoker_test
    invoker_test.cpp
    )
target_link_libraries(invoker_test
    actor
    )

addtest(account_actor_test
    account_actor_test.cpp
    )
target_link_libraries(account_actor_test
    actor
    state_tree
    ipfs_datastore_in_memory
    )
