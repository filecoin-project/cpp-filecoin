/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {

  class MockIpfsDatastore : public IpfsDatastore {
   public:
    MOCK_CONST_METHOD1(contains, outcome::result<bool>(const CID &key));
    MOCK_METHOD2(setMock, outcome::result<void>(const CID &key, BytesIn value));
    outcome::result<void> set(const CID &key, BytesCow &&value) override {
      return setMock(key, value);
    }
    MOCK_CONST_METHOD1(get, outcome::result<Value>(const CID &key));
    MOCK_METHOD1(remove, outcome::result<void>(const CID &key));
    MOCK_METHOD0(shared, IpldPtr());

    bool operator==(const MockIpfsDatastore &) const {
      return true;
    }
  };

}  // namespace fc::storage::ipfs
