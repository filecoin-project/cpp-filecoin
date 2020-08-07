/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/fwd.hpp"

namespace fc::blocksync {
  using libp2p::Host;
  using libp2p::peer::PeerInfo;
  using primitives::tipset::Tipset;

  enum class Error {
    kInconsistent = -1,
    kOk = 0,
    kPartial = 101,
    kNotFound = 201,
    kGoAway = 202,
    kInternalError = 203,
    kBadRequest = 204,
  };

  // TODO: depth/count
  using Cb = std::function<void(outcome::result<Tipset>)>;
  void fetch(std::shared_ptr<Host> host,
             const PeerInfo &peer,
             IpldPtr ipld,
             std::vector<CID> blocks,
             Cb cb);

  void serve(std::shared_ptr<Host> host, IpldPtr ipld);
}  // namespace fc::blocksync

OUTCOME_HPP_DECLARE_ERROR(fc::blocksync, Error)
