#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(discovery_test
    discovery_test.cpp
    )
target_link_libraries(discovery_test
    address
    discovery
    in_memory_storage
    )
