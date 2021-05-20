#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(light_reader)

addtest(cbor_test
    cbor_test.cpp
    )
target_link_libraries(cbor_test
    cbor
    hexutil
    )

addtest(cbor_buffering_test
    cbor_buffering_test.cpp
    )
target_link_libraries(cbor_buffering_test
    cbor_stream
    )
