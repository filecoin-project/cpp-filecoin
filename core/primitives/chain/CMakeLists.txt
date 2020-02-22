#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_chain
    chain.cpp
    )
target_link_libraries(filecoin_chain
    filecoin_outcome
    filecoin_tickets
    )
