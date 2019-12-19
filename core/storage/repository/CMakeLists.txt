# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(repository
    repository.cpp
    )
target_link_libraries(repository
    outcome
    keystore
    ipfs
    )
