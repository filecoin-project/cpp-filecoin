#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(deal_info_manager_test
    deal_info_manager_test.cpp
    )
target_link_libraries(deal_info_manager_test
    deal_info_manager
    )
