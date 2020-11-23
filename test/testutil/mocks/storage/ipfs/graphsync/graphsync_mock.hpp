/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESTUTILS_MOCK_GRAPHSYNC_MOCK_HPP
#define CPP_FILECOIN_TESTUTILS_MOCK_GRAPHSYNC_MOCK_HPP

#include "storage/ipfs/graphsync/graphsync.hpp"

#include <gmock/gmock.h>

namespace fc::storage::ipfs::graphsync {

  class GraphsyncMock : public Graphsync {
   public:
    MOCK_METHOD1(subscribe,
                 DataConnection(std::function<OnDataReceived>));

    MOCK_METHOD0(start,
                 void());

    MOCK_METHOD1(setDefaultRequestHandler,
        void(std::function<RequestHandler>));

    MOCK_METHOD2(setRequestHandler,
                 void(std::function<RequestHandler>, std::string));

    MOCK_METHOD2(postResponse, void(const FullRequestId &id,
                              const Response &response));

    MOCK_METHOD0(stop, void());

    MOCK_METHOD6(
        makeRequest,
        Subscription(const libp2p::peer::PeerId &peer,
                     boost::optional<libp2p::multi::Multiaddress> address,
                     const CID &root_cid,
                     gsl::span<const uint8_t> selector,
                     const std::vector<Extension> &extensions,
                     Graphsync::RequestProgressCallback callback));
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_TESTUTILS_MOCK_GRAPHSYNC_MOCK_HPP
