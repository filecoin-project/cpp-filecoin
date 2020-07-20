/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocksync_client.hpp"

#include <libp2p/host/host.hpp>

#include "storage/ipfs/datastore.hpp"

namespace fc::sync::blocksync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
      return logger.get();
    }
  }  // namespace

  BlocksyncClient::BlocksyncClient(
      std::shared_ptr<libp2p::Host> host,
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld)
      : host_(std::move(host)), ipld_(std::move(ipld)) {
    assert(host_);
    assert(ipld_);
  }

  void BlocksyncClient::init(OnBlockStored block_cb, OnPeerFeedback peer_cb) {
    assert(block_cb);
    assert(peer_cb);

    block_cb_ = std::move(block_cb);
    peer_cb_ = std::move(peer_cb);

    initialized_ = true;
  }

  outcome::result<void> BlocksyncClient::makeRequest(const PeerId &peer,
                                                     std::vector<CID> blocks,
                                                     uint64_t depth,
                                                     RequestOptions options) {
    if (!initialized_) {
      return Error::SYNC_NOT_INITIALIZED;
    }

    if (blocks.empty()) {
      return outcome::success();
    }

    if (options == MESSAGES_ONLY) {
      // not supported yet
      return Error::SYNC_MSG_LOAD_FAILURE;
    }

    auto counter = ++request_counter_;

    requests_.insert(
        {counter, Ctx{Request{std::move(blocks), depth, options}, {}, peer}});

    host_->newStream(
        // peer must be already connected
        libp2p::peer::PeerInfo{peer, {}},
        protocol_id,
        [wptr = weak_from_this(), counter](auto rstream) {
          auto self = wptr.lock();
          if (self) {
            if (rstream) {
              if (self->initialized_) {
                self->onConnected(
                    counter, std::make_shared<CborStream>(rstream.value()));
              } else {
                rstream.value()->reset();
              }
            } else if (self->initialized_) {
              self->onConnected(counter, rstream.error());
            }
          }
        });

    return outcome::success();
  }

  void BlocksyncClient::stop() {
    if (!initialized_) {
      return;
    }

    initialized_ = false;

    for (auto &[_, ctx] : requests_) {
      if (ctx.stream) {
        ctx.stream->stream()->reset();
      }
    }

    requests_.clear();
  }

  void BlocksyncClient::onConnected(uint64_t counter,
                                    outcome::result<StreamPtr> rstream) {
    auto it = requests_.find(counter);
    if (it == requests_.end()) {
      if (rstream) {
        rstream.value()->stream()->reset();
      }
      return;
    }

    auto &ctx = it->second;

    std::error_code e;
    common::Buffer serialized;

    if (!rstream) {
      e = rstream.error();
    } else {
      auto res = codec::cbor::encode<Request>(ctx.request);
      if (res) {
        serialized = std::move(res.value());
      } else {
        e = res.error();
      }
    }

    if (!e) {
      ctx.stream = std::move(rstream.value());

      // clang-format off
      ctx.stream->stream()->write(
        serialized,
        serialized.size(),
        [wptr = weak_from_this(), buf = std::move(serialized), counter]
        (auto res) {
           auto self = wptr.lock();
           if (self) {
              self->onRequestWritten(counter, res);
           }
        }
      );
      // clang-format on

    } else {
      if (rstream) {
        rstream.value()->stream()->reset();
      }
      closeRequest(it, e);
    }
  }

  void BlocksyncClient::onRequestWritten(uint64_t counter,
                                         outcome::result<size_t> result) {
    if (!initialized_) {
      return;
    }

    auto it = requests_.find(counter);
    if (it == requests_.end()) {
      return;
    }

    if (!result) {
      closeRequest(it, result.error());
      return;
    }

    assert(it->second.stream);

    it->second.stream->read<Response>(
        [wptr = weak_from_this(), counter](auto res) {
          auto self = wptr.lock();
          if (self) {
            self->onResponseRead(counter, std::move(res));
          }
        });
  }

  void BlocksyncClient::onResponseRead(uint64_t counter,
                                       outcome::result<Response> result) {
    if (!initialized_) {
      return;
    }

    auto it = requests_.find(counter);
    if (it == requests_.end()) {
      return;
    }

    if (!result) {
      closeRequest(it, result.error());
      return;
    }

    auto &ctx = it->second;
    auto &response = result.value();

    std::error_code peer_feedback_code;

    switch (response.status) {
      case ResponseStatus::RESPONSE_COMPLETE:
        break;
      case ResponseStatus::RESPONSE_PARTIAL:
        peer_feedback_code = Error::SYNC_INCOMPLETE_BLOCKSYNC_RESPONSE;
        break;
      default:
        peer_feedback_code = Error::SYNC_BLOCKSYNC_RESPONSE_ERROR;
        break;
    }

    if (!response.chain.empty()) {
      auto res = storeResponse(ipld_,
                               response.chain,
                               ctx.request.options == BLOCKS_AND_MESSAGES,
                               block_cb_);
      if (!res) {
        closeRequest(it, res.error());
        return;
      }
    }

    peer_cb_(ctx.peer, peer_feedback_code);
    requests_.erase(it);
  }

  void BlocksyncClient::closeRequest(Requests::iterator it,
                                     std::error_code error) {
    assert(error);

    auto ctx = std::move(it->second);
    requests_.erase(it);

    if (ctx.stream) {
      ctx.stream->stream()->reset();
      peer_cb_(ctx.peer, error);
    }

    for (auto &cid : ctx.request.block_cids) {
      block_cb_(std::move(cid), error);
    }
  }

}  // namespace fc::sync::blocksync
