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

add_library(sector_counter
        impl/sector_counter_impl.cpp
        )
target_link_libraries(sector_counter
        stored_counter
        outcome
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
        )

add_library(events
        impl/events_impl.cpp
        )
target_link_libraries(events
        logger
        tipset_cache
        chain_store
        )

add_library(storage_fsm
        impl/sealing_impl.cpp
        impl/checks.cpp
        )
target_link_libraries(storage_fsm
        fuhon_fsm
        logger
        address
        tipset
        api
        events
        precommit_policy
        sector_stat
        sector_counter
        api_ipfs_datastore
        zerocomm
        )
