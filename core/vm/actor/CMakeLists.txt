#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(actor
    account_actor.cpp
    actor.cpp
    init_actor.cpp
    cron_actor.cpp
    impl/invoker_impl.cpp
    )
target_link_libraries(actor
    address
    cid
    exit_code
    hamt
    )
