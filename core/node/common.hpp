/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

#include "crypto/signature/signature.hpp"
#include "node/events_fwd.hpp"
#include "primitives/big_int.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::sync {
  using crypto::signature::Signature;
  using libp2p::peer::PeerId;
  using primitives::BigInt;
  using primitives::block::BlockHeader;
  using primitives::block::BlockWithCids;
  using primitives::block::BlockWithMessages;
  using primitives::block::SignedMessage;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetHash;
  using primitives::tipset::TipsetKey;
  using vm::message::UnsignedMessage;
}  // namespace fc::sync
