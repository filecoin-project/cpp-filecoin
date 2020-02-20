/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_TYPES_HPP
#define CPP_FILECOIN_GRAPHSYNC_TYPES_HPP

#include <cstdint>
#include <memory>

#include <gsl/span>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/common/types.hpp>

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"

namespace libp2p::connection {
  class Stream;
}

namespace fc::storage::ipfs::graphsync {

  using libp2p::common::ByteArray;
  using SharedData = std::shared_ptr<const ByteArray>;
  using libp2p::peer::PeerId;
  using SessionPtr = std::shared_ptr<struct Session>;
  using StreamPtr = std::shared_ptr<libp2p::connection::Stream>;

  constexpr std::string_view kResponseMetadata = "graphsync/response-metadata";
  constexpr std::string_view kDontSendCids = "graphsync/do-not-send-cids";
  constexpr std::string_view kLink = "link";
  constexpr std::string_view kBlockPresent = "blockPresent";
  constexpr std::string_view kProtocolVersion = "/ipfs/graphsync/1.0.0";

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_TYPES_HPP
