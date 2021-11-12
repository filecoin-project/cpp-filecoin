#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#


add_library(deal_info_manager
    impl/deal_info_manager_impl.cpp
    )
target_link_libraries(deal_info_manager
    logger
    tipset
    api
    )
