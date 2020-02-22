#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_builtin_shared
    shared.cpp
    )
target_link_libraries(filecoin_builtin_shared
    filecoin_actor
    filecoin_address
    filecoin_runtime
    )
