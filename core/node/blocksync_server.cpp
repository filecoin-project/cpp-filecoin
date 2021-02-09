/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocksync_server.hpp"

#include <libp2p/host/host.hpp>

#include "common/logger.hpp"
#include "primitives/tipset/load.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::sync::blocksync {

  namespace {
    constexpr size_t kBlockSyncMaxRequestLength = 800;

    auto log() {
      static common::Logger logger = common::createLogger("blocksync-server");
      return logger.get();
    }

    std::string peerStr(
        const std::shared_ptr<libp2p::connection::Stream> &stream) {
      if (stream) {
        try {
          return fmt::format(
              "{}/p2p/{}",
              stream->remoteMultiaddr().value().getStringAddress(),
              stream->remotePeerId().value().toBase58());
        } catch (...) {
        }
      }
      return "(dead stream)";
    }

    bool isValidRequest(const outcome::result<Request> &request) {
      if (!request) {
        log()->debug("invalid request: {}", request.error().message());
        return false;
      }
      auto &r = request.value();
      if (r.block_cids.empty()) {
        log()->debug("invalid request: no block cids");
        return false;
      }
      if (r.depth == 0) {
        log()->debug("invalid request: zero depth");
        return false;
      }
      if ((r.options & BLOCKS_AND_MESSAGES) == 0) {
        log()->debug("invalid request: bad options");
        return false;
      }
      return true;
    }

    template <typename T>
    struct MessageVisitor {
      outcome::result<void> operator()(size_t, const CID &cid) {
        auto index{visited.find(cid)};
        if (index == visited.end()) {
          index = visited.emplace(cid, messages.size()).first;
          OUTCOME_TRY(message, ipld->getCbor<T>(cid));
          messages.push_back(std::move(message));
        }
        indices.rbegin()->push_back(index->second);
        return outcome::success();
      }

      IpldPtr &ipld;
      std::vector<T> &messages;
      MsgIncudes &indices;
      std::map<CID, size_t> visited{};
    };

    void getChain(TsLoadPtr ts_load,
                  IpldPtr ipld,
                  const Request &request,
                  Response &response) {
      bool partial = false;
      size_t depth = request.depth;
      if (request.depth > kBlockSyncMaxRequestLength) {
        partial = true;
        depth = kBlockSyncMaxRequestLength;
      }

      try {
        OUTCOME_EXCEPT(ts, ts_load->load(request.block_cids));
        while (true) {
          TipsetBundle packed;
          if (request.options & MESSAGES_ONLY) {
            TipsetBundle::Messages msgs;
            MessageVisitor<UnsignedMessage> bls_visitor{
                ipld, msgs.bls_msgs, msgs.bls_msg_includes};
            MessageVisitor<SignedMessage> secp_visitor{
                ipld, msgs.secp_msgs, msgs.secp_msg_includes};
            for (auto &block : ts->blks) {
              OUTCOME_EXCEPT(
                  meta,
                  ipld->getCbor<primitives::block::MsgMeta>(block.messages));
              msgs.bls_msg_includes.emplace_back();
              OUTCOME_EXCEPT(meta.bls_messages.visit(bls_visitor));
              msgs.secp_msg_includes.emplace_back();
              OUTCOME_EXCEPT(meta.secp_messages.visit(secp_visitor));
            }
            packed.messages = std::move(msgs);
          }
          if (request.options & BLOCKS_ONLY) {
            packed.blocks = ts->blks;
          }
          response.chain.push_back(std::move(packed));
          if (response.chain.size() >= depth) {
            break;
          }
          if (ts->height() == 0) {
            partial = false;
            break;
          }
          OUTCOME_EXCEPT(parent, ts_load->load(ts->getParents()));
          ts = std::move(parent);
        }
      } catch (const std::system_error &e) {
        log()->debug("failed filling response: {}", e.code().message());
      }

      if (response.chain.empty()) {
        response.status = ResponseStatus::BLOCK_NOT_FOUND;
        response.message = "not found";
      } else {
        response.status = partial ? ResponseStatus::RESPONSE_PARTIAL
                                  : ResponseStatus::RESPONSE_COMPLETE;
      }
    }

  }  // namespace

  BlocksyncServer::BlocksyncServer(std::shared_ptr<libp2p::Host> host,
                                   TsLoadPtr ts_load,
                                   IpldPtr ipld)
      : host_(std::move(host)),
        ts_load_(std::move(ts_load)),
        ipld_(std::move(ipld)) {
    assert(host_);
  }

  void BlocksyncServer::start() {
    if (started_) {
      return;
    }

    host_->setProtocolHandler(
        kProtocolId,
        [wptr{weak_from_this()}](
            outcome::result<std::shared_ptr<libp2p::connection::Stream>>
                rstream) {
          auto self = wptr.lock();
          if (!self) {
            return;
          }

          if (!rstream) {
            log()->debug("incoming stream error: {}",
                         rstream.error().message());
            return;
          }

          if (!self->started_) {
            log()->debug("not started, ignoring stream from {}",
                         peerStr(rstream.value()));
            rstream.value()->reset();
            return;
          }

          log()->debug("reading request from {}", peerStr(rstream.value()));
          auto stream{std::make_shared<common::libp2p::CborStream>(
              std::move(rstream.value()))};

          stream->template read<Request>(
              [stream, wptr](outcome::result<Request> request) {
                auto self = wptr.lock();
                if (!self) {
                  return;
                }
                self->onRequest(std::move(stream), std::move(request));
              });
        });

    started_ = true;
    log()->info("started");
  }

  void BlocksyncServer::stop() {
    log()->info("stopped");
    started_ = false;
  }

  void BlocksyncServer::onRequest(StreamPtr stream,
                                  outcome::result<Request> request) {
    Response response;
    if (started_) {
      if (isValidRequest(request)) {
        log()->debug("request from {}: depth={}",
                     peerStr(stream->stream()),
                     request.value().depth);
        getChain(ts_load_, ipld_, request.value(), response);
      } else {
        response.status = ResponseStatus::BAD_REQUEST;
        response.message = "bad request";
      }
    } else {
      response.status = ResponseStatus::GO_AWAY;
      response.message = "blocksync server stopped";
    }
    stream->write(response, [stream](auto) {
      log()->debug("response written to {}", peerStr(stream->stream()));
      stream->close();
    });
  }

}  // namespace fc::sync::blocksync
