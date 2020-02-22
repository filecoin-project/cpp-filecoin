#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_runtime
    impl/env.cpp
    impl/runtime_impl.cpp
    impl/actor_state_handle_impl.cpp
    impl/runtime_error.cpp)
target_link_libraries(filecoin_runtime
    filecoin_actor
    p2p::p2p_cid
    filecoin_randomness_provider
    )
