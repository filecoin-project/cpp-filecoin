#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_fsm
        impl/sealing_impl.cpp
        )
target_link_libraries(storage_fsm
        fuhon_fsm
        logger
        )
