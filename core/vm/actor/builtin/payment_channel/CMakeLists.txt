#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_payment_channel_actor
    payment_channel_actor.cpp
    )
target_link_libraries(filecoin_payment_channel_actor
    filecoin_actor
    filecoin_outcome
    filecoin_signature
    )
