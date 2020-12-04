/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_COMMON_HPP
#define CPP_FILECOIN_SYNC_COMMON_HPP

#include "events_fwd.hpp"

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

#include "primitives/big_int.hpp"
#include "crypto/signature/signature.hpp"
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
  using primitives::tipset::TipsetKey;
  using vm::message::UnsignedMessage;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetHash;
  using BranchId = uint64_t;
  using Height = uint64_t;

  constexpr BranchId kNoBranch = 0;
  constexpr BranchId kGenesisBranch = 1;
}

#endif  // CPP_FILECOIN_SYNC_COMMON_HPP
