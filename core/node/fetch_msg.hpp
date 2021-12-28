/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>

#include "node/blocksync_request.hpp"
#include "node/peer_height.hpp"

namespace fc::sync::fetch_msg {
  using blocksync::BlocksyncRequest;
  using libp2p::Host;
  using libp2p::basic::Scheduler;
  using peer_height::PeerHeight;
  using primitives::ChainEpoch;

  constexpr size_t kQueueMax{1000};
  constexpr size_t kFetchingMax{20};

  struct FetchMsg {
    std::shared_ptr<Host> host;
    std::shared_ptr<Scheduler> scheduler;
    std::shared_ptr<PeerHeight> peers;
    IpldPtr ipld;
    std::set<std::pair<ChainEpoch, TipsetKey>> queue;
    std::unordered_map<TipsetKey, std::shared_ptr<BlocksyncRequest>>
        fetching_tsk;
    std::unordered_set<PeerId> fetching_peer;
    std::function<void(TipsetKey)> on_fetch;

    FetchMsg(std::shared_ptr<Host> host,
             std::shared_ptr<Scheduler> scheduler,
             std::shared_ptr<PeerHeight> peers,
             IpldPtr ipld)
        : host{host}, scheduler{scheduler}, peers{peers}, ipld{ipld} {}

    bool has(const TipsetCPtr &ts, bool priority) {
      for (const auto &block : ts->blks) {
        if (!ipld->contains(block.messages).value()) {
          if (queue.size() >= kQueueMax) {
            queue.erase(std::prev(queue.end()));
          }
          queue.emplace(priority ? -1 : ts->height(), ts->key);
          dequeue();
          return false;
        }
      }
      return true;
    }

    void dequeue() {
      if (queue.empty()) {
        return;
      }
      if (fetching_tsk.size() >= kFetchingMax) {
        return;
      }
      auto it{queue.begin()};
      // note: BlocksyncRequests caches duplicate requests
      peers->visit(it->first, [&](const PeerId &peer) {
        if (fetching_peer.count(peer) != 0) {
          return true;
        }
        auto tsk{std::move(it->second)};
        queue.erase(it);
        fetching_peer.emplace(peer);
        fetching_tsk.emplace(
            tsk,
            BlocksyncRequest::newRequest(
                *host,
                *scheduler,
                ipld,
                nullptr,
                peer,
                tsk.cids(),
                1,
                blocksync::kMessagesOnly,
                10000,
                [this](BlocksyncRequest::Result r) { onFetch(std::move(r)); }));
        return false;
      });
    }

    void onFetch(BlocksyncRequest::Result &&r) {
      TipsetKey tsk{std::move(r.blocks_requested)};
      fetching_tsk.erase(tsk);
      fetching_peer.erase(*r.from);
      if (r.error) {
        peers->onError(*r.from);
      }
      dequeue();
      if (on_fetch) {
        on_fetch(std::move(tsk));
      }
    }
  };
}  // namespace fc::sync::fetch_msg
