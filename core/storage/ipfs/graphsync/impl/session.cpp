/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spdlog/fmt/fmt.h>

#include "session.hpp"

namespace fc::storage::ipfs::graphsync {

  namespace {

    /// String representation for loggers and debug purposes
    std::string makeStringRepr(uint64_t session_id, const PeerId &peer_id) {
      return fmt::format("{}-{}", session_id, peer_id.toBase58().substr(46));
    }

  }  // namespace

  Session::Session(uint64_t session_id, PeerId peer_id,
                   Session::State initial_state)
      : id(session_id),
        peer(std::move(peer_id)),
        str(makeStringRepr(id, peer)),
        state(initial_state) {}

}  // namespace fc::storage::ipfs::graphsync
