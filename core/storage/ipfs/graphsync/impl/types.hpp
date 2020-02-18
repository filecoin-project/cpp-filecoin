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

#include "common/buffer.hpp"
#include "common/outcome.hpp"

namespace libp2p::connection {
  class Stream;
}

namespace fc::storage::ipfs::graphsync {

  using SharedData = std::shared_ptr<const common::Buffer>;
  using libp2p::peer::PeerId;
  using SessionPtr = std::shared_ptr<struct Session>;
  using StreamPtr = std::shared_ptr<libp2p::connection::Stream>;

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_TYPES_HPP
