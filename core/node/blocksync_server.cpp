/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/blocksync_server.hpp"

#include <libp2p/host/host.hpp>

#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
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
      if ((r.options & kBlocksAndMessages) == 0) {
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
          OUTCOME_TRY(message, getCbor<T>(ipld, cid));
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

      auto _result{[&]() -> outcome::result<void> {
        OUTCOME_TRY(ts, ts_load->load(request.block_cids));
        while (true) {
          TipsetBundle packed;
          if (request.options & kMessagesOnly) {
            TipsetBundle::Messages msgs;
            MessageVisitor<UnsignedMessage> bls_visitor{
                ipld, msgs.bls_msgs, msgs.bls_msg_includes};
            MessageVisitor<SignedMessage> secp_visitor{
                ipld, msgs.secp_msgs, msgs.secp_msg_includes};
            for (auto &block : ts->blks) {
              OUTCOME_TRY(
                  meta,
                  getCbor<primitives::block::MsgMeta>(ipld, block.messages));
              msgs.bls_msg_includes.emplace_back();
              OUTCOME_TRY(meta.bls_messages.visit(bls_visitor));
              msgs.secp_msg_includes.emplace_back();
              OUTCOME_TRY(meta.secp_messages.visit(secp_visitor));
            }
            packed.messages = std::move(msgs);
          }
          if (request.options & kBlocksOnly) {
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
          OUTCOME_TRY(parent, ts_load->load(ts->getParents()));
          ts = std::move(parent);
        }
        return outcome::success();
      }()};
      if (!_result) {
        log()->debug("failed filling response: {:#}", _result.error());
      }

      if (response.chain.empty()) {
        response.status = ResponseStatus::kBlockNotFound;
        response.message = "not found";
      } else {
        response.status = partial ? ResponseStatus::kResponsePartial
                                  : ResponseStatus::kResponseComplete;
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
        response.status = ResponseStatus::kBadRequest;
        response.message = "bad request";
      }
    } else {
      response.status = ResponseStatus::kGoAway;
      response.message = "blocksync server stopped";
    }
    stream->write(response, [stream](auto) {
      log()->debug("response written to {}", peerStr(stream->stream()));
      stream->close();
    });
  }

}  // namespace fc::sync::blocksync
