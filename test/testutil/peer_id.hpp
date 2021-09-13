/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multihash.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "common/buffer.hpp"
#include "common/outcome.hpp"

using fc::common::Buffer;
using libp2p::multi::Multihash;
using libp2p::peer::PeerId;

/**
 * Creates dummy PeerId
 */
inline PeerId generatePeerId(int value) {
  Buffer kBuffer = Buffer(43, value);
  auto hash = Multihash::create(libp2p::multi::sha256, kBuffer).value();
  return PeerId::fromHash(hash).value();
}
