#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(json_test
        json_test.cpp
        )

target_link_libraries(json_test
        api
        json
        address
        rle_plus_codec
        )
