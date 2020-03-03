/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_IPFS_IPFS_DATASTORE_MOCK_
#define CPP_FILECOIN_STORAGE_IPFS_IPFS_DATASTORE_MOCK_

#include <gmock/gmock.h>

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {

  class MockIpfsDatastore : public IpfsDatastore {
   public:
    MOCK_CONST_METHOD1(contains, outcome::result<bool>(const CID &key));
    MOCK_METHOD2(set, outcome::result<void>(const CID &key, Value value));
    MOCK_CONST_METHOD1(get, outcome::result<Value>(const CID &key));
    MOCK_METHOD1(remove, outcome::result<void>(const CID &key));

    bool operator==(const MockIpfsDatastore &) const {
      return true;
    }
  };

}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_STORAGE_IPFS_IPFS_DATASTORE_MOCK_
