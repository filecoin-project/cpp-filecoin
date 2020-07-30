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
      std::shared_ptr<blocksync::BlocksyncClient> blocksync)
      : ipld_(std::move(ipld)),
        scheduler_(std::move(scheduler)),
        blocksync_(std::move(blocksync)) {
    assert(ipld_);
    assert(scheduler_);
    assert(blocksync_);
  }

  void BlockLoader::init(OnBlockSynced callback) {
    assert(callback);
    callback_ = std::move(callback);
    blocksync_->init(
        [this](CID block_cid, outcome::result<BlockMsg> result) {
          onBlockStored(std::move(block_cid), std::move(result));
        },
        [](const PeerId &peer, std::error_code error) {
          // TODO (artem) handle and forward this (to choose another peer)
          if (error) {
            log()->error("peer {}: {}", peer.toBase58(), error.message());
          }
        });
    initialized_ = true;
  }

  outcome::result<BlockLoader::BlocksAvailable> BlockLoader::loadBlocks(
      const std::vector<CID> &cids,
      boost::optional<PeerId> preferred_peer,
      uint64_t load_parents_depth) {
    if (!initialized_) {
      return Error::SYNC_NOT_INITIALIZED;
    }

    if (preferred_peer.has_value()) {
      if (!last_peer_.has_value()
          || last_peer_.value() != preferred_peer.value()) {
        last_peer_ = std::move(preferred_peer);
      }
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
      if (last_peer_.has_value()) {
        uint64_t depth = load_parents_depth;
        if (depth == 0) {
          depth = 1;
        } else if (depth > 50) {
          depth = 50;
        }
        OUTCOME_TRY(blocksync_->makeRequest(last_peer_.value(),
                                            std::move(wanted_),
                                            depth,
                                            blocksync::BLOCKS_AND_MESSAGES));
      } else {
        return Error::SYNC_NO_PEERS;
      }
    }
    return blocks_available;
  }

  void BlockLoader::onBlockStored(CID block_cid,
                                  outcome::result<BlockMsg> result) {
    bool was_requested = false;
    if (!result) {
      log()->error("blocksync failure, cid: {}, error: {}",
                   block_cid.toString().value(),
                   result.error().message());
      was_requested = onBlockHeader(block_cid, boost::none, false);
    } else {
      BlockMsg &m = result.value();
      was_requested = onBlockHeader(block_cid, std::move(m.header), true);
    }
    if (!was_requested) {
      log()->trace("block cid {} was not requested",
                   block_cid.toString().value());
    }
  }

  bool BlockLoader::onBlockHeader(const CID &cid,
                                  boost::optional<BlockHeader> header,
                                  bool block_completed) {
    auto it = block_requests_.find(cid);
    if (it == block_requests_.end()) {
      return false;
    }
    it->second->onBlockHeader(std::move(header), block_completed);
    return true;
  }

  outcome::result<BlockLoader::BlockAvailable>
  BlockLoader::findBlockInLocalStore(const CID &cid) {
    BlockAvailable ret;

    auto header_available = ipld_->getCbor<BlockHeader>(cid);
    if (!header_available) {
      if (header_available.error()
          != storage::ipfs::IpfsDatastoreError::kNotFound) {
        return Error::SYNC_BAD_BLOCK;
      }
      return ret;
    }
    ret.header = std::move(header_available.value());

    auto meta_available = ipld_->getCbor<MsgMeta>(ret.header->messages);
    if (!meta_available) {
      if (meta_available.error()
          != storage::ipfs::IpfsDatastoreError::kNotFound) {
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

    wanted_.push_back(cid);

    // TODO (artem): restore previous code to load messages for block

    return boost::none;
  }

  void BlockLoader::onRequestCompleted(CID block_cid,
                                       boost::optional<BlockHeader> bh) {
    assert(callback_);
    block_requests_.erase(block_cid);

    if (bh.has_value()) {
      log()->info("request completed for block {} with height={}",
                  block_cid.toString().value(),
                  bh.value().height);
      callback_(block_cid, std::move(bh.value()));
    } else {
      callback_(block_cid, Error::SYNC_BAD_BLOCK);  // TODO more errors
    }
  }

  BlockLoader::RequestCtx::RequestCtx(BlockLoader &o, const CID &cid)
      : owner(o), block_cid(cid) {}

  void BlockLoader::RequestCtx::onBlockHeader(boost::optional<BlockHeader> bh,
                                              bool block_completed) {
    if (is_bad || header.has_value()) {
      return;
    }

    if (!bh) {
      is_bad = true;
      call_completed = owner.scheduler_->schedule(
          [this]() { owner.onRequestCompleted(block_cid, std::move(header)); });
    }

    header = std::move(bh);

    if (block_completed) {
      call_completed = owner.scheduler_->schedule(
          [this]() { owner.onRequestCompleted(block_cid, std::move(header)); });
    }

    // TODO (artem) wait for messages : separate loader
  }

}  // namespace fc::sync
