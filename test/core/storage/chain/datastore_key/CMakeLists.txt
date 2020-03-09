# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_datastore_key_cbor_test
    datastore_key_cbor_test.cpp
    )
target_link_libraries(filecoin_datastore_key_cbor_test
    filecoin_cbor
    filecoin_datastore_key
    )

addtest(filecoin_datastore_key_compare_test
    datastore_key_compare_test.cpp
    )
target_link_libraries(filecoin_datastore_key_compare_test
    filecoin_datastore_key
    )

addtest(filecoin_datastore_key_create_test
    datastore_key_create_test.cpp
    )
target_link_libraries(filecoin_datastore_key_create_test
    filecoin_datastore_key
    )
