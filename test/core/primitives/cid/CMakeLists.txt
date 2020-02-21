# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(cid_json_test
    cid_json_test.cpp
    )
target_link_libraries(cid_json_test
    cid
    )
