# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(tipset
    tipset.cpp
    )
target_link_libraries(tipset
    cid
    )
