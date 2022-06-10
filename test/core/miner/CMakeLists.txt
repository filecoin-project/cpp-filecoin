#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(events_test
    events_test.cpp
    )
target_link_libraries(events_test
    events
    )

addtest(batcher_test
    precommit_batcher_test.cpp
    commit_batcher_test.cpp
    )
target_link_libraries(batcher_test
    batcher
    p2p::p2p_manual_scheduler_backend
    )

addtest(precommit_policy_test
    precommit_policy_test.cpp
    )
target_link_libraries(precommit_policy_test
    precommit_policy)

addtest(sector_stat_test
    sector_stat_test.cpp
    )
target_link_libraries(sector_stat_test
    sector_stat)

addtest(tipset_cache_test
    tipset_cache_test.cpp
    )
target_link_libraries(tipset_cache_test
    tipset_cache
    )

addtest(sealing_test
    sealing_test.cpp
    sealing_mark_for_upgrade_test.cpp
    checks_test.cpp
    )
target_link_libraries(sealing_test
    storage_fsm
    ipfs_datastore_in_memory
    in_memory_storage
    miner_actor_state
    p2p::p2p_basic_scheduler
    p2p::p2p_manual_scheduler_backend
    )

