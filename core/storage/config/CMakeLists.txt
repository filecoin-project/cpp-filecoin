# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_config
    config.cpp
    config_error.cpp
    )
target_link_libraries(filecoin_config
    filecoin_outcome
    )
