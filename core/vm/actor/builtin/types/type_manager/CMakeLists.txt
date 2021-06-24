#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(type_manager
    type_manager.cpp
    )
target_link_libraries(type_manager
    array
    miner_types
    )
