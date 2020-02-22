# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_in_memory_storage
    in_memory_storage.cpp
    )
target_link_libraries(filecoin_in_memory_storage
    filecoin_buffer
    )
