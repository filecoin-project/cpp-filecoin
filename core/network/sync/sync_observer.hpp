/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_OBSERVER_HPP
#define CPP_FILECOIN_SYNC_OBSERVER_HPP

#include "common/buffer.hpp"
#include "primitives/big_int.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/message/message.hpp"

namespace fc::network::sync {
  using common::Buffer;
  using crypto::signature::Signature;
  using primitives::BigInt;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  struct SyncState {
    bool synchronized;  ///< allows to mine blocks
    TipsetKey head;     ///< current local node's head
    //    uint64_t target_height;  ///< best height discovered over the network
    BigInt target_weight;  ///< best weight discovered over the network
  };

  struct SyncObserver {
    virtual ~SyncObserver() = default;

    virtual void onSyncStateChanged(const SyncState &newState) = 0;
  };

  struct BlockObserver {
    virtual ~BlockObserver() = default;

    virtual void onNewBlock(const BlockHeader &blk) = 0;
  };

  struct MessageObserver {
    virtual ~MessageObserver() = default;

    virtual void onNewMessage(const UnsignedMessage &m,
                              boost::optional<Signature> signature) = 0;
  };

}  // namespace fc::network::sync

#endif  // CPP_FILECOIN_SYNC_OBSERVER_HPP
