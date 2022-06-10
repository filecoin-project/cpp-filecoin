#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sector_stat
        impl/sector_stat_impl.cpp
        )
target_link_libraries(sector_stat
        sector
        )

add_library(precommit_policy
        impl/basic_precommit_policy.cpp
        )
target_link_libraries(precommit_policy
        Boost::boost
        api
        )

add_library(tipset_cache
        impl/tipset_cache_impl.cpp
        )
target_link_libraries(tipset_cache
        tipset
        Boost::boost
        api
        )

add_library(events
        impl/events_impl.cpp
        )
target_link_libraries(events
        logger
        tipset_cache
        api
        )

add_library(batcher
        impl/precommit_batcher_impl.cpp
        impl/commit_batcher_impl.cpp
        )
target_link_libraries(batcher
        api
        )

add_library(storage_fsm
        impl/sealing_impl.cpp
        impl/checks.cpp
        )
target_link_libraries(storage_fsm
        logger
        address
        tipset
        api
        events
        precommit_policy
        sector_stat
        stored_counter
        api_ipfs_datastore
        zerocomm
        sector_file
        deal_info_manager
        batcher
        )
