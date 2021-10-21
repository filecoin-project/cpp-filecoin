/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multihash.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "common/bytes.hpp"
#include "common/outcome.hpp"

using fc::Bytes;
using libp2p::multi::Multihash;
using libp2p::peer::PeerId;

/**
 * Creates dummy PeerId
 */
inline PeerId generatePeerId(int value) {
  Bytes kBuffer = Bytes(43, value);
  auto hash = Multihash::create(libp2p::multi::sha256, kBuffer).value();
  return PeerId::fromHash(hash).value();
}
