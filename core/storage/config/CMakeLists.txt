# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(config
    config.cpp
    )
target_link_libraries(config
    filestore
    outcome
    )
