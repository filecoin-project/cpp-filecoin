# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(compacter
    compacter.cpp
    queue.cpp
    )
target_link_libraries(compacter
    cids_index
    interpreter
    map_prefix
    tipset
    )
