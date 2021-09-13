/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/chain/chain_data_store.hpp"

namespace fc::storage::blockchain {
  class ChainDataStoreMock: public ChainDataStore {
   public:
    MOCK_CONST_METHOD1(contains, outcome::result<bool>(const DatastoreKey &key));
    MOCK_METHOD2(set, outcome::result<void>(const DatastoreKey &key, std::string_view value));
    MOCK_CONST_METHOD1(get, outcome::result<Value>(const DatastoreKey &key));
    MOCK_METHOD1(remove, outcome::result<void>(const DatastoreKey &key));
  };
}
