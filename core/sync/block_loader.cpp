/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "block_loader.hpp"

#include <cassert>

#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::sync {
  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
      return logger.get();
    }
  }  // namespace

  BlockLoader::BlockLoader(
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      std::shared_ptr<ObjectLoader> object_loader)
      : ipld_(std::move(ipld)),
        scheduler_(std::move(scheduler)),
        object_loader_(std::move(object_loader)) {
    assert(ipld_);
    assert(scheduler_);
    assert(object_loader_);
  }

  void BlockLoader::init(OnBlockSynced callback) {
    assert(callback);
    callback_ = std::move(callback);
    object_loader_->init(
        [this](const CID &cid,
               bool object_is_valid,
               boost::optional<BlockHeader> header) {
          return onBlockHeader(cid, object_is_valid, std::move(header));
        },
        [this](const CID &cid,
               bool object_is_valid,
               const std::vector<CID> &bls_messages,
               const std::vector<CID> &secp_messages) {
          return onMsgMeta(cid, object_is_valid, bls_messages, secp_messages);
        },
        [this](const CID &cid, bool is_secp_message, bool object_is_valid) {
          return onMessage(cid, is_secp_message, object_is_valid);
        });
    initialized_ = true;
  }

  outcome::result<BlockLoader::BlocksAvailable> BlockLoader::loadBlocks(
      const std::vector<CID> &cids,
      boost::optional<PeerId> preferred_peer) {
    if (!initialized_) {
      return Error::SYNC_NOT_INITIALIZED;
    }

    BlocksAvailable blocks_available;
    if (cids.empty()) {
      return blocks_available;
    }
    blocks_available.resize(cids.size());

    wanted_.clear();

    size_t idx = 0;
    for (const auto &cid : cids) {
      OUTCOME_TRY(header, tryLoadBlock(cid));

      if (header.has_value()) {
        // all subobjects are in local storage
        blocks_available[idx] = std::move(header);
      }

      ++idx;
    }

    if (!wanted_.empty()) {
      OUTCOME_TRY(object_loader_->loadObjects(std::move(wanted_),
                                              std::move(preferred_peer)));
    }
    return blocks_available;
  }

  bool BlockLoader::onBlockHeader(const CID &cid,
                                  bool object_is_valid,
                                  boost::optional<BlockHeader> header) {
    auto it = block_requests_.find(cid);
    if (it == block_requests_.end()) {
      return false;
    }
    it->second->onBlockHeader(object_is_valid, std::move(header));
    return object_is_valid;
  }

  bool BlockLoader::onMsgMeta(const CID &cid,
                              bool object_is_valid,
                              const std::vector<CID> &bls_messages,
                              const std::vector<CID> &secp_messages) {
    auto range = meta_requests_.equal_range(cid);
    if (range.first == range.second) {
      return false;
    }

    if (!object_is_valid || (bls_messages.empty() && secp_messages.empty())) {
      for (auto it = range.first; it != range.second; ++it) {
        it->second->onMeta(object_is_valid, true);
      }
    } else {
      Wantlist bls_wantlist;
      Wantlist secp_wantlist;
      wanted_.clear();

      for (const auto &m : bls_messages) {
        auto contains = ipld_->contains(m);
        if (!contains || !contains.value()) {  // TODO !contains is db error
          if (!msg_requests_.count(cid)) {
            wanted_.push_back({cid, ObjectLoader::BLS_MESSAGE});
            bls_wantlist.insert(cid);
          }
        }
      }

      for (const auto &m : secp_messages) {
        auto contains = ipld_->contains(m);
        if (!contains || !contains.value()) {
          if (!msg_requests_.count(cid)) {
            wanted_.push_back({cid, ObjectLoader::SECP_MESSAGE});
            secp_wantlist.insert(cid);
          }
        }
      }

      if (wanted_.empty()) {
        for (auto it = range.first; it != range.second; ++it) {
          it->second->onMeta(object_is_valid, true);
        }
      } else {
        bool cannot_load = false;
        auto res = object_loader_->loadObjects(wanted_, boost::none);
        if (!res) {
          log()->error("onMeta error: {}", res.error().message());
          cannot_load = true;
        }

        for (auto it = range.first; it != range.second; ++it) {
          auto &ctx = it->second;
          if (cannot_load) {
            ctx->onMeta(false, true);
          } else {
            for (const auto &w : wanted_) {
              msg_requests_.insert({w.cid, ctx});
            }
            ctx->bls_messages = bls_wantlist;
            ctx->secp_messages = secp_wantlist;
            ;
          }
        }

        wanted_.clear();
      }
    }

    meta_requests_.erase(range.first, range.second);
    return object_is_valid;
  }

  bool BlockLoader::onMessage(const CID &cid,
                              bool is_secp_message,
                              bool object_is_valid) {
    auto range = msg_requests_.equal_range(cid);
    if (range.first == range.second) {
      return false;
    }
    for (auto it = range.first; it != range.second; ++it) {
      it->second->onMessage(cid, is_secp_message, object_is_valid);
    }
    msg_requests_.erase(range.first, range.second);
    return object_is_valid;
  }

  outcome::result<BlockLoader::BlockAvailable>
  BlockLoader::findBlockInLocalStore(const CID &cid) {
    BlockAvailable ret;

    auto header_available = ipld_->getCbor<BlockHeader>(cid);
    if (!header_available) {
      if (header_available.error()
          != storage::ipfs::IpfsDatastoreError::NOT_FOUND) {
        return Error::SYNC_BAD_BLOCK;
      }
      return ret;
    }
    ret.header = std::move(header_available.value());

    auto meta_available = ipld_->getCbor<MsgMeta>(ret.header->messages);
    if (!meta_available) {
      if (meta_available.error()
          != storage::ipfs::IpfsDatastoreError::NOT_FOUND) {
        return Error::SYNC_BAD_BLOCK;
      }
      return ret;
    }

    ret.meta_available = true;

    auto &meta = meta_available.value();
    OUTCOME_TRY(meta.bls_messages.visit(
        [&, this](auto, auto &cid) -> outcome::result<void> {
          OUTCOME_TRY(contains, ipld_->contains(cid));
          if (!contains) {
            ret.bls_messages_to_load.insert(std::move(cid));
          }
          return outcome::success();
        }));
    OUTCOME_TRY(meta.secp_messages.visit(
        [&, this](auto, auto &cid) -> outcome::result<void> {
          OUTCOME_TRY(contains, ipld_->contains(cid));
          if (!contains) {
            ret.secp_messages_to_load.insert(std::move(cid));
          }
          return outcome::success();
        }));

    if (ret.bls_messages_to_load.empty() && ret.secp_messages_to_load.empty()) {
      ret.all_messages_available = true;
    }

    return ret;
  }

  outcome::result<boost::optional<BlockHeader>> BlockLoader::tryLoadBlock(
      const CID &cid) {
    if (block_requests_.count(cid) != 0) {
      return boost::none;
    }

    auto res = findBlockInLocalStore(cid);
    if (!res) {
      return res.error();
    }

    auto &info = res.value();

    if (info.all_messages_available) {
      return info.header;
    }

    auto ctx = std::make_shared<RequestCtx>(*this, cid);
    block_requests_[cid] = ctx;

    if (info.header.has_value()) {
      ctx->header = std::move(info.header);

      if (!info.meta_available) {
        meta_requests_.insert({ctx->header->messages, ctx});
        wanted_.push_back({ctx->header->messages, ObjectLoader::MSG_META});

      } else {
        for (const auto &m : info.bls_messages_to_load) {
          msg_requests_.insert({m, ctx});
          wanted_.push_back({m, ObjectLoader::BLS_MESSAGE});
        }
        ctx->bls_messages = std::move(info.bls_messages_to_load);

        for (const auto &m : info.secp_messages_to_load) {
          msg_requests_.insert({m, ctx});
          wanted_.push_back({m, ObjectLoader::SECP_MESSAGE});
        }
        ctx->secp_messages = std::move(info.secp_messages_to_load);
      }

    } else {
      wanted_.push_back({cid, ObjectLoader::BLOCK_HEADER});
    }

    return boost::none;
  }

  void BlockLoader::onRequestCompleted(CID block_cid,
                                       boost::optional<BlockHeader> bh) {
    assert(callback_);
    block_requests_.erase(block_cid);
    if (bh.has_value()) {
      callback_(block_cid, std::move(bh.value()));
    } else {
      callback_(block_cid, Error::SYNC_BAD_BLOCK);  // TODO more errors
    }
  }

  BlockLoader::RequestCtx::RequestCtx(BlockLoader &o, const CID &cid)
      : owner(o), block_cid(cid) {}

  void BlockLoader::RequestCtx::onBlockHeader(bool object_is_valid,
                                              boost::optional<BlockHeader> bh) {
    if (is_bad || header.has_value()) {
      return;
    }

    if (!object_is_valid) {
      is_bad = true;
      call_completed = owner.scheduler_->schedule(
          [this]() { owner.onRequestCompleted(block_cid, std::move(header)); });
    }

    header = std::move(bh);
  }

  void BlockLoader::RequestCtx::onMeta(bool object_is_valid,
                                       bool no_more_messages) {
    if (is_bad || !header.has_value()) {
      return;
    }

    bool completed = false;

    if (!object_is_valid) {
      is_bad = true;
      header = boost::none;
      completed = true;
    } else if (no_more_messages) {
      completed = true;
    }

    if (completed) {
      call_completed = owner.scheduler_->schedule(
          [this]() { owner.onRequestCompleted(block_cid, std::move(header)); });
    }
  }

  void BlockLoader::RequestCtx::onMessage(const CID &cid,
                                          bool is_secp,
                                          bool object_is_valid) {
    if (is_bad || !header.has_value()) {
      return;
    }

    bool completed = false;

    Wantlist &wantlist = is_secp ? secp_messages : bls_messages;
    wantlist.erase(cid);

    if (!object_is_valid) {
      is_bad = true;
      header = boost::none;
      completed = true;
    } else {
      completed = bls_messages.empty() && secp_messages.empty();
    }

    if (completed) {
      call_completed = owner.scheduler_->schedule(
          [this]() { owner.onRequestCompleted(block_cid, std::move(header)); });
    }
  }

}  // namespace fc::sync
