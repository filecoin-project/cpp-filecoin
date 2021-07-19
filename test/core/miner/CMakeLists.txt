#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(events_test
        events_test.cpp
        )
target_link_libraries(events_test
        events)

addtest(batcher_test
        precommit_batcher_test.cpp
        )
target_link_libraries(batcher_test
        batcher)

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
        tipset_cache)

addtest(deal_info_manager_test
        deal_info_manager_test.cpp
        )
target_link_libraries(deal_info_manager_test
        deal_info_manager
        )

addtest(sealing_test
        sealing_test.cpp
        checks_test.cpp
        )
target_link_libraries(sealing_test
        storage_fsm
        ipfs_datastore_in_memory
        in_memory_storage
        miner_actor_v0
        miner_actor_v2
        miner_actor_v3
        )

