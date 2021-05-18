#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(node_test
    node_test.cpp
    )
target_link_libraries(node_test
    node_builder
    )
