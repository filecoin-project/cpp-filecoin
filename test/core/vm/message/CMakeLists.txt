#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_message_test
    message_test.cpp
    )
target_link_libraries(filecoin_message_test
    filecoin_message
    )
