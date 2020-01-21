# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(tipset
    tipset.cpp
    tipset_key.cpp
    )
target_link_libraries(tipset
    buffer
    cid
    )
