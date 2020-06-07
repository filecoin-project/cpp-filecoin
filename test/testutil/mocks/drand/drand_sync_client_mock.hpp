/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_DRAND_DRAND_SYNC_CLIENT_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_DRAND_DRAND_SYNC_CLIENT_MOCK_HPP

#include <gmock/gmock.h>

#include "drand/client.hpp"

fc::drand {
  class DrandSyncClientMock : public DrandSyncClient {
   public:
    MOCK_METHOD1(publicRand,
                 outcome::result<PublicRandResponse>(uint64_t round));

    MOCK_METHOD1(
        publicRandStream,
        outcome::result<std::vector<PublicRandResponse>>(uint64_t round));

    MOCK_METHOD0(group, outcome::result<GroupPacket>())
  };
};

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_DRAND_DRAND_SYNC_CLIENT_MOCK_HPP
