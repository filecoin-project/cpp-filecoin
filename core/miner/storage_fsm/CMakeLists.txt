#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

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
        )
target_link_libraries(storage_fsm
        fuhon_fsm
        logger
        address
        tipset
        api
        events
        )
