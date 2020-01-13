# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(ticket_test
    ticket_test.cpp
    )
target_link_libraries(ticket_test
    ticket_utils
    )
