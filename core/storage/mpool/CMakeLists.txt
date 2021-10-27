# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(mpool
    mpool.cpp
    )
target_link_libraries(mpool
    message
    sync
    )
