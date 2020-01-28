#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(message_test_util
    message_test_util.cpp
    )
target_link_libraries(message_test_util
    message
    )
