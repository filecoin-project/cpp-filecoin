#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(tipset_cache
        impl/tipset_cache_impl.cpp
        )
target_link_libraries(tipset_cache
        logger
        )

add_library(events
        impl/events_impl.cpp
        )
target_link_libraries(events
        logger
        tipset_cache
        )

add_library(storage_fsm
        impl/sealing_impl.cpp
        )
target_link_libraries(storage_fsm
        fuhon_fsm
        logger
        api
        events
        )
