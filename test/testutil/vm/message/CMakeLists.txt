#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_message_test_util
    message_test_util.cpp
    )
target_link_libraries(filecoin_message_test_util
    filecoin_message
    )
