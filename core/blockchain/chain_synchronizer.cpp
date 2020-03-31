/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/chain_synchronizer.hpp"
#include <boost/assert.hpp>
#include "codec/cbor/cbor.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::blockchain {
  using primitives::cid::getCidOfCbor;
  using storage::ipfs::InMemoryDatastore;

  ChainSynchronizer::ChainSynchronizer(io_context &context,
                                       std::weak_ptr<SyncManager> sync_manager,
                                       std::shared_ptr<Sync> sync,
                                       std::shared_ptr<ChainStore> store)
      : context_{context},
        sync_manager_(std::move(sync_manager)),
        sync_(std::move(sync)),
        chain_store_{std::move(store)} {
    setState(ChainSynchronizerState::STATE_INIT);
    std::shared_ptr<IpfsDatastore> base = chain_store_;
    std::shared_ptr<IpfsDatastore> diff = std::make_shared<InMemoryDatastore>();
    blocks_batch_ = std::make_shared<Batch>(std::move(base), std::move(diff));
    logger_ = common::createLogger("ChainSynchronizer");
  }
  void ChainSynchronizer::start(const TipsetKey &head, uint64_t limit) {
    limit_ = limit;
    chain_.push_back(head);

    for (const auto &cid : head.cids) {
      loadBlock(cid);
    }
  }

  void ChainSynchronizer::cancel() {
    stopDownloading();
  }

  void ChainSynchronizer::stopDownloading() {
    for (auto &ticket : block_tickets_) {
      sync_->cancelLoading(ticket.first);
    }

    for (auto &ticket : signed_msg_tickets_) {
      sync_->cancelLoading(ticket.first);
    }

    for (auto &ticket : unsigned_msg_tickets_) {
      sync_->cancelLoading(ticket.first);
    }
  }

  void ChainSynchronizer::loadBlock(const CID &cid) {
    blocks_.insert(cid);
    auto ticket =
        sync_->load(cid, [this](const auto &r) { onObjectReceived(r); });
    block_tickets_[ticket] = cid;
  }

  void ChainSynchronizer::loadSigMessage(const CID &cid) {
    signed_msgs_.insert(cid);
    auto ticket =
        sync_->load(cid, [this](const auto &r) { onObjectReceived(r); });
    signed_msg_tickets_[ticket] = cid;
  }

  void ChainSynchronizer::loadUnsigMessage(const CID &cid) {
    unsigned_msgs_.insert(cid);
    auto ticket =
        sync_->load(cid, [this](const auto &r) { onObjectReceived(r); });
    unsigned_msg_tickets_[ticket] = cid;
  }

  ObjectType ChainSynchronizer::findObject(LoadTicket t, const CID &cid) const {
    bool is_block = block_tickets_.find(t) != block_tickets_.end();
    bool is_sigmsg = signed_msg_tickets_.find(t) != signed_msg_tickets_.end();
    bool is_unsigmsg =
        unsigned_msg_tickets_.find(t) != unsigned_msg_tickets_.end();

    if (!is_block and !is_sigmsg and !is_unsigmsg) {
      OUTCOME_ALTERNATIVE(cid_value, cid.toString(), "<failed to format CID>");
      logger_->error(
          "unwanted object received ticket = {}, cid = {}", t, cid_value);
      return ObjectType::BAD;
    }

    if ((static_cast<int>(is_block) + static_cast<int>(is_sigmsg)
         + static_cast<int>(is_unsigmsg))
        != 1) {
      OUTCOME_ALTERNATIVE(cid_value, cid.toString(), "<failed to format cid>");
      logger_->error(
          "internal error: object is in different categories, ticket = {} cid "
          "= {}",
          t,
          cid_value);
      return ObjectType::BAD;
    }

    if (is_block) {
      return ObjectType::BLOCK;
    }

    if (is_unsigmsg) {
      return ObjectType::UNSIGNED_MESSAGE;
    }

    if (is_sigmsg) {
      return ObjectType::SIGNED_MESSAGE;
    }

    return ObjectType::MISSING;
  }

  void ChainSynchronizer::retryLoad(LoadTicket t) {
    // TODO(yuraz) : maybe add retry limit later

    auto handle_object = [this](const Sync::LoadResult &r) {
      onObjectReceived(r);
    };

    if (auto it = block_tickets_.find(t); it != block_tickets_.end()) {
      sync_->load(it->second, handle_object);
    }
  }

  void ChainSynchronizer::onObjectReceived(const Sync::LoadResult &r) {
    auto object_type = findObject(r.ticket, r.cid);

    if (!r.data) {
      // retry
      logger_->error("failed to download object: {}", r.data.error().message());
      retryLoad(r.ticket);
      return;
    }

    if (object_type == ObjectType::BLOCK) {
      // get data
      auto &&block_data = r.data.value();
      BOOST_ASSERT_MSG(block_data != nullptr,
                       "received block data is null pointer");
      // decode
      auto &&block = codec::cbor::decode<BlockHeader>(*block_data);
      if (!block) {
        OUTCOME_ALTERNATIVE(
            cid_value, r.cid.toString(), "<failed to convert cid>");
        logger_->error("failed to decode block, cid = {}", cid_value);
      }
      // if (!header) // process error
      // add to storage
      auto &&res = blocks_batch_->set(r.cid, *r.data.value());
      if (!res) {
      }

      block_tickets_.erase(r.ticket);
    }

    //    if (auto it = std::find(
    //            signed_msg_tickets_.begin(),
    //            signed_msg_tickets_.end(),
    //            [t = r.ticket](const auto &pair) { return pair.first == t; });
    //        it != signed_msg_tickets_.end()) {
    //      // decode
    //      // process result
    //      // add to storage
    //      signed_msg_tickets_.erase(it);
    //    }

    //    if (auto it = std::find(unsigned_msg_tickets_.begin(),
    //                            unsigned_msg_tickets_.end(),
    //                            r.ticket);
    //        it != unsigned_msg_tickets_.end()) {
    //      // decode
    //      // process result
    //      // add to storage
    //      unsigned_msg_tickets_.erase(it);
    //    }

    // update sync state
    updateState();

    // unwanted object received, report error
  }

  void ChainSynchronizer::updateState() {}
}  // namespace fc::blockchain