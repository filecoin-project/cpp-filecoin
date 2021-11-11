#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(paych_maker
    maker.cpp
    )
target_link_libraries(paych_maker
    map_prefix
    # TODO(turuslan): extract `getPaymentChannelCodeId` from toolchain
    toolchain
    )

add_library(paych_vouchers
    vouchers.cpp
    )
target_link_libraries(paych_vouchers
    map_prefix
    payment_channel_actor_state
    )
