#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(chain_events
    impl/chain_events_impl.cpp
    )
target_link_libraries(chain_events
    api
    )
