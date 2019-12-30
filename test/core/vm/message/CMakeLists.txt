#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(message_test
    message_test.cpp
    )
target_link_libraries(message_test
    message
    )
