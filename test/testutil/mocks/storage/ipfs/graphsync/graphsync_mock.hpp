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
    MOCK_METHOD2(start,
                 void(std::shared_ptr<MerkleDagBridge> dag,
                      Graphsync::BlockCallback callback));

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
