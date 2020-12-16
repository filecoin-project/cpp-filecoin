/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tipset_request.hpp"

#include "blocksync_request.hpp"
#include "chain_db.hpp"
#include "common/logger.hpp"

namespace fc::sync {
  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("tipset_loader");
      return logger.get();
    }
  }  // namespace

  class TipsetRequest::Impl
      : public std::enable_shared_from_this<TipsetRequest::Impl> {
   public:
    Impl(ChainDb &db) : db_(db) {}

    void makeRequest(libp2p::Host &host,
                     libp2p::protocol::Scheduler &scheduler,
                     Ipld &ipld,
                     PeerId peer,
                     std::vector<CID> blocks,
                     uint64_t depth,
                     uint64_t timeoutMsec,
                     bool index_head_tipset,
                     std::function<void(Result)> callback) {
      index_head_tipset_ = index_head_tipset;
      callback_ = std::move(callback);
      request_.emplace(
          host,
          scheduler,
          ipld,
          std::move(peer),
          std::move(blocks),
          depth,
          blocksync::BLOCKS_AND_MESSAGES,
          timeoutMsec + 5000,
          [wptr = weak_from_this(),
           this](blocksync::BlocksyncRequest::Result r) {
            if (wptr.expired()) {
              return;
            }
            indexAndForward(std::move(r));
          });
    }

    void cancel() {
      assert(request_);
      request_->cancel();
    }

   private:
    void indexAndForward(blocksync::BlocksyncRequest::Result r) {
      Result result{.from = std::move(r.from),
                    .delta_rating = r.delta_rating,
                    .error = r.error};

      TipsetCPtr head;

      if (!r.blocks_available.empty()) {
        if (index_head_tipset_) {
          auto ts = Tipset::create(std::move(r.blocks_available));
          if (!ts) {
            result.error = blocksync::BlocksyncRequest::Error::
                BLOCKSYNC_INCONSISTENT_RESPONSE;
            result.delta_rating -= 500;
            callback_(std::move(result));
            return;
          }
          head = std::move(ts.value());
        }

        try {
          bool proceed = true;

          if (head) {
            proceed = indexTipset(result, head);
            result.head = std::move(head);
            result.head_indexed = true;
          }

          for (auto &tipset : r.parents) {
            if (!proceed) {
              break;
            }
            proceed = indexTipset(result, tipset);
          }

        } catch (const std::system_error &e) {
          log()->error("tipset store error {}", e.code().message());
          result.delta_rating -= 150;
        }
      }

      callback_(std::move(result));
    }

    /// returns true to proceed indexing parents
    bool indexTipset(Result &result, TipsetCPtr tipset) {
      TipsetKey parent = tipset->getParents();
      OUTCOME_EXCEPT(sync_state, db_.storeTipset(tipset, parent));
      if (!result.head) {
        result.head = tipset;
      }
      if (sync_state.unsynced_bottom) {
        result.next_target_height = sync_state.unsynced_bottom->height() - 1;
        if (*sync_state.unsynced_bottom == *tipset) {
          // continue indexing parents
          result.next_target = std::move(parent);
          return true;
        }
        result.next_target = sync_state.unsynced_bottom->getParents();
      }
      return false;
    }

    ChainDb &db_;

    /// The option
    bool index_head_tipset_ = false;

    std::function<void(Result)> callback_;

    /// Blocksync request. Other protocols can be added here
    boost::optional<blocksync::BlocksyncRequest> request_;
  };

  TipsetRequest::TipsetRequest(ChainDb &db,
                               libp2p::Host &host,
                               libp2p::protocol::Scheduler &scheduler,
                               Ipld &ipld,
                               PeerId peer,
                               std::vector<CID> blocks,
                               uint64_t depth,
                               uint64_t timeoutMsec,
                               bool index_head_tipset,
                               std::function<void(Result)> callback) {
    assert(callback);
    impl_ = std::make_shared<Impl>(db);

    // need shared_from_this there
    impl_->makeRequest(host,
                       scheduler,
                       ipld,
                       std::move(peer),
                       std::move(blocks),
                       depth,
                       timeoutMsec,
                       index_head_tipset,
                       std::move(callback));
  }

  TipsetRequest::~TipsetRequest() {
    cancel();
  }

  void TipsetRequest::cancel() {
    if (impl_) {
      impl_->cancel();
      impl_.reset();
    }
  }
}  // namespace fc::sync
