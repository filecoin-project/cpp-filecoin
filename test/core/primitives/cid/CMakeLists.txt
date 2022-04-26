# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(cid_test
    cid_test.cpp
    )
target_link_libraries(cid_test
    cid
    )
