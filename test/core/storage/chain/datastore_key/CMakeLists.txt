# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(datastore_key_cbor_test
    datastore_key_cbor_test.cpp
    )
target_link_libraries(datastore_key_cbor_test
    cbor
    datastore_key
    )

addtest(datastore_key_compare_test
    datastore_key_compare_test.cpp
    )
target_link_libraries(datastore_key_compare_test
    datastore_key
    )

addtest(datastore_key_create_test
    datastore_key_create_test.cpp
    )
target_link_libraries(datastore_key_create_test
    datastore_key
    )
