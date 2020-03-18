#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(api_json_test
    json_test.cpp
    )
target_link_libraries(api_json_test
    rpc
    )
