#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(builtin_shared
    shared.cpp
    )
target_link_libraries(builtin_shared
    actor
    address
    runtime
    )
