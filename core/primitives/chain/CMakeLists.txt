#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(chain
    chain.cpp
    )
target_link_libraries(chain
    outcome
    tickets
    )
