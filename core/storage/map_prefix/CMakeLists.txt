# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(map_prefix
    prefix.cpp
    )
target_link_libraries(map_prefix
    outcome
    )
