#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_init_actor
    init_actor.cpp
    )
target_link_libraries(filecoin_init_actor
    filecoin_actor
    filecoin_outcome
    filecoin_hamt
    )
