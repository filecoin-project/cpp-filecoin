/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/host.hpp>

#include "common/libp2p/cbor_stream.hpp"
#include "node/blocksync.hpp"
#include "primitives/tipset/tipset.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

OUTCOME_CPP_DEFINE_CATEGORY(fc::blocksync, Error, e) {
  return "fc::blocksync::Error";
}

namespace fc::blocksync {
  using common::libp2p::CborStream;
  using primitives::block::BlockHeader;
  using primitives::block::MsgMeta;
  using primitives::block::SignedMessage;
  using primitives::block::UnsignedMessage;

  static constexpr auto kProtocolId{"/fil/sync/blk/0.0.1"};
  constexpr size_t kBlockSyncMaxRequestLength{800};

  struct Request {
    enum Options {
      BLOCKS = 1,
      MESSAGES = 2,
      BLOCKS_AND_MESSAGES = BLOCKS | MESSAGES,
    };

    std::vector<CID> blocks;
    size_t depth{1};
    Options options{BLOCKS_AND_MESSAGES};
  };
  CBOR_TUPLE(Request, blocks, depth, options)

  struct Response {
    struct Tipset {
      using Indices = std::vector<std::vector<size_t>>;

      std::vector<BlockHeader> blocks;
      std::vector<UnsignedMessage> bls_messages;
      Indices bls_indices;
      std::vector<SignedMessage> secp_messages;
      Indices secp_indices;
    };

    std::vector<Tipset> chain;
    Error status;
    std::string message;
  };
  CBOR_TUPLE(Response, chain, status, message)
  CBOR_TUPLE(Response::Tipset,
             blocks,
             bls_messages,
             bls_indices,
             secp_messages,
             secp_indices)

  outcome::result<Tipset> unpack(const IpldPtr &ipld, Response::Tipset packed) {
    auto safe{[&](auto &messages, auto &indices) {
      if (indices.size() != packed.blocks.size()) {
        return false;
      }
      for (auto &indices2 : indices) {
        if (!indices2.empty()
            && *std::max_element(indices2.begin(), indices2.end())
                   >= messages.size()) {
          return false;
        }
      }
      return true;
    }};
    if (!safe(packed.bls_messages, packed.bls_indices)
        || !safe(packed.secp_messages, packed.secp_indices)) {
      return Error::kInconsistent;
    }
    std::vector<BlockHeader> blocks;
    for (auto &block : packed.blocks) {
      OUTCOME_TRY(ipld->setCbor(block));
      blocks.push_back(std::move(block));
    }
    std::vector<CID> bls_cids, secp_cids;
    for (auto &message : packed.bls_messages) {
      OUTCOME_TRY(cid, ipld->setCbor(message));
      bls_cids.push_back(std::move(cid));
    }
    for (auto &message : packed.secp_messages) {
      OUTCOME_TRY(cid, ipld->setCbor(message));
      secp_cids.push_back(std::move(cid));
    }
    auto i{0};
    for (auto &block : blocks) {
      MsgMeta messages;
      ipld->load(messages);
      for (auto &j : packed.bls_indices[i]) {
        OUTCOME_TRY(messages.bls_messages.append(bls_cids[j]));
      }
      for (auto &j : packed.secp_indices[i]) {
        OUTCOME_TRY(messages.secp_messages.append(secp_cids[j]));
      }
      OUTCOME_TRY(cid, ipld->setCbor(messages));
      if (cid != block.messages) {
        return Error::kInconsistent;
      }
      ++i;
    }
    return Tipset::create(blocks);
  }

  void fetch(std::shared_ptr<Host> host,
             const PeerInfo &peer,
             IpldPtr ipld,
             std::vector<CID> blocks,
             Cb cb) {
    host->newStream(
        peer, kProtocolId, [MOVE(ipld), MOVE(blocks), MOVE(cb)](auto _stream) {
          if (!_stream) {
            return cb(_stream.error());
          }
          auto stream{std::make_shared<CborStream>(_stream.value())};
          stream->write(
              Request{std::move(blocks)},
              [stream, MOVE(ipld), MOVE(cb)](auto _n) {
                if (!_n) {
                  stream->close();
                  return cb(_n.error());
                }
                stream->template read<Response>(
                    [stream, MOVE(ipld), MOVE(cb)](auto _response) {
                      stream->close();
                      if (!_response) {
                        return cb(_response.error());
                      }
                      auto &response{_response.value()};
                      if (response.status != Error::kOk) {
                        return cb(response.status);
                      }
                      if (response.chain.empty()) {
                        return cb(Error::kPartial);
                      }
                      cb(unpack(ipld, std::move(response.chain.front())));
                    });
              });
        });
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
    Response::Tipset::Indices &indices;
    std::map<CID, size_t> visited{};
  };

  outcome::result<std::vector<Response::Tipset>> getChain(
      IpldPtr ipld, const Request &request) {
    OUTCOME_TRY(ts, Tipset::load(*ipld, request.blocks));
    std::vector<Response::Tipset> chain;
    while (true) {
      Response::Tipset packed;
      if (request.options & Request::MESSAGES) {
        MessageVisitor<UnsignedMessage> bls_visitor{
            ipld, packed.bls_messages, packed.bls_indices};
        MessageVisitor<SignedMessage> secp_visitor{
            ipld, packed.secp_messages, packed.secp_indices};
        for (auto &block : ts.blks) {
          OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
          packed.bls_indices.emplace_back();
          OUTCOME_TRY(meta.bls_messages.visit(bls_visitor));
          packed.secp_indices.emplace_back();
          OUTCOME_TRY(meta.secp_messages.visit(secp_visitor));
        }
      }
      if (request.options & Request::BLOCKS) {
        packed.blocks = ts.blks;
      }
      chain.push_back(std::move(packed));
      if (chain.size() >= request.depth || ts.height == 0) {
        break;
      }
      OUTCOME_TRYA(ts, ts.loadParent(*ipld));
    }
    return chain;
  }

  void serve(std::shared_ptr<Host> host, IpldPtr ipld) {
    host->setProtocolHandler(kProtocolId, [MOVE(ipld)](auto _stream) {
      auto stream{std::make_shared<CborStream>(_stream)};
      stream->template read<Request>([stream, MOVE(ipld)](auto _request) {
        Response response;
        if (_request) {
          auto &request{_request.value()};
          if (request.blocks.empty()) {
            response.status = Error::kBadRequest;
            response.message = "no cids given in blocksync request";
          } else {
            auto partial{request.depth > kBlockSyncMaxRequestLength};
            if (partial) {
              request.depth = kBlockSyncMaxRequestLength;
            }
            auto _chain{getChain(ipld, request)};
            if (_chain) {
              response.chain = std::move(_chain.value());
              response.status = partial ? Error::kPartial : Error::kOk;
            } else {
              response.status = Error::kInternalError;
              response.message = _chain.error().message();
            }
          }
        } else {
          response.status = Error::kBadRequest;
          response.message = _request.error().message();
        }
        stream->write(response, [stream](auto) { stream->close(); });
      });
    });
  }
}  // namespace fc::blocksync
