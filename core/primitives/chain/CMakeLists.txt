#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(chain
    chain.cpp
    )
target_link_libraries(chain
    outcome
    tickets
    )

add_library(chain_tips_manager
    chain_tips_manager.cpp
    )
target_link_libraries(chain_tips_manager
    chain
    )
