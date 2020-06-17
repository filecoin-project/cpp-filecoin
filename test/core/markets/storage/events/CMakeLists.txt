#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(events_test
    events_test.cpp
    )
target_link_libraries(events_test
    events
    ipfs_datastore_in_memory
    )
