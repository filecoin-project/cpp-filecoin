/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocksync_client.hpp"

#include <libp2p/host/host.hpp>

#include "common/logger.hpp"
#include "events.hpp"
#include "storage/ipfs/datastore.hpp"

#define TRACE_ENABLED 0

namespace fc::sync::blocksync {

  namespace {
    using primitives::block::MsgMeta;

    auto log() {
      static common::Logger logger = common::createLogger("blocksync_client");
      return logger.get();
    }

    template <typename... Args>
    inline void trace(spdlog::string_view_t fmt, const Args &... args) {
#if TRACE_ENABLED
      log()->trace(fmt, args...);
#endif
    }

    bool findBlockInLocalStore(const CID &cid,
                               storage::ipfs::IpfsDatastore &ipld,
                               events::Events &events) {
      auto header = ipld.getCbor<BlockHeader>(cid);
      if (!header) {
        return false;
      }

      auto meta = ipld.getCbor<MsgMeta>(header.value().messages);
      if (!meta) {
        return false;
      }

      bool all_messages_available = true;

      BlockWithCids block;

      auto res = meta.value().bls_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            if (all_messages_available) {
              OUTCOME_TRY(contains, ipld.contains(cid));
              if (!contains) {
                all_messages_available = false;
              }
              block.bls_messages.push_back(cid);
            }
            return outcome::success();
          });

      if (!res || !all_messages_available) {
        return false;
      }

      res = meta.value().secp_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            if (all_messages_available) {
              OUTCOME_TRY(contains, ipld.contains(cid));
              if (!contains) {
                all_messages_available = false;
              }
              block.secp_messages.push_back(cid);
            }
            return outcome::success();
          });

      if (!res || !all_messages_available) {
        return false;
      }

      block.header = std::move(header.value());

      events.signalBlockStored({.from = boost::none,
                                .block_cid = cid,
                                .block = std::move(block),
                                .messages_stored = true});

      return true;
    }

    std::vector<CID> tryReduceRequest(std::vector<CID> blocks,
                                      storage::ipfs::IpfsDatastore &ipld,
                                      events::Events &events) {
      std::vector<CID> reduced;
      reduced.reserve(blocks.size());
      for (auto& cid : blocks) {
        if (findBlockInLocalStore(cid, ipld, events)) {
          reduced.push_back(std::move(cid));
        }
      }
      return reduced;
    }

    void storeBlock(PeerId from,
                    const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
                    BlockHeader header,
                    const std::vector<CID> &secp_cids,
                    const std::vector<uint64_t> &secp_includes,
                    const std::vector<CID> &bls_cids,
                    const std::vector<uint64_t> &bls_includes,
                    bool store_messages,
                    events::Events &events,
                    std::unordered_set<CID> &waitlist) {
      events::BlockStored event{
          .from = std::move(from),
          .block_cid = CID{},
          .block = BlocksyncClient::Error::BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH,
          .messages_stored = false};

      try {
        BlockWithCids m{std::move(header), {}, {}};

        OUTCOME_EXCEPT(block_cid, ipld->setCbor<BlockHeader>(m.header));
        event.block_cid = std::move(block_cid);

        if (!waitlist.empty()) {
          waitlist.erase(event.block_cid);
        }

        if (store_messages) {
          MsgMeta meta;
          ipld->load(meta);

          m.secp_messages.reserve(secp_includes.size());
          for (auto idx : secp_includes) {
            m.secp_messages.push_back(secp_cids[idx]);
            OUTCOME_EXCEPT(meta.secp_messages.append(secp_cids[idx]));
          }

          m.bls_messages.reserve(bls_includes.size());
          for (auto idx : bls_includes) {
            m.bls_messages.push_back(bls_cids[idx]);
            OUTCOME_EXCEPT(meta.bls_messages.append(bls_cids[idx]));
          }

          OUTCOME_EXCEPT(meta_cid, ipld->setCbor<MsgMeta>(meta));

          if (meta_cid != m.header.messages) {
            throw std::system_error(
                BlocksyncClient::Error::BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH);
          }
        }

        event.block = std::move(m);
        event.messages_stored = store_messages;

      } catch (const std::system_error &e) {
        event.block = e.code();
      }

      events.signalBlockStored(std::move(event));
    }

    void storeTipsetBundle(
        const PeerId &from,
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
        TipsetBundle &bundle,
        bool store_messages,
        events::Events &events,
        std::unordered_set<CID> &waitlist) {
      size_t sz = bundle.blocks.size();

      trace(
          "storing tipset bundle of {} blocks, {} bls messages, {} secp "
          "messages",
          sz,
          bundle.messages.bls_msgs.size(),
          bundle.messages.secp_msgs.size());

      std::vector<CID> secp_cids;
      std::vector<CID> bls_cids;

      try {
        if (store_messages) {
          if (bundle.messages.secp_msg_includes.size() != sz
              || bundle.messages.bls_msg_includes.size() != sz) {
            throw std::system_error(
                BlocksyncClient::Error::BLOCKSYNC_INCONSISTENT_RESPONSE);
          }

          secp_cids.reserve(bundle.messages.secp_msgs.size());
          for (const auto &msg : bundle.messages.secp_msgs) {
            OUTCOME_EXCEPT(
                cid, ipld->setCbor<primitives::block::SignedMessage>(msg));
            secp_cids.push_back(std::move(cid));
          }

          bls_cids.reserve(bundle.messages.bls_msgs.size());
          for (const auto &msg : bundle.messages.bls_msgs) {
            OUTCOME_EXCEPT(cid, ipld->setCbor<UnsignedMessage>(msg));
            bls_cids.push_back(std::move(cid));
          }
        }
      } catch (const std::system_error &e) {
        log()->error("cannot store tipset bundle received from peer {}: {}",
                     from.toBase58(),
                     e.code().message());
        return;
      }

      for (size_t i = 0; i < sz; ++i) {
        storeBlock(from,
                   ipld,
                   std::move(bundle.blocks[i]),
                   secp_cids,
                   bundle.messages.secp_msg_includes[i],
                   bls_cids,
                   bundle.messages.bls_msg_includes[i],
                   store_messages,
                   events,
                   waitlist);
      }
    }

    void storeResponse(
        const PeerId &from,
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
        std::vector<TipsetBundle> chain,
        bool store_messages,
        events::Events &events,
        std::unordered_set<CID> &waitlist) {
      log()->debug("storing {} tipset bundles from peer {}",
                   chain.size(),
                   from.toBase58());

      for (auto &bundle : chain) {
        storeTipsetBundle(from, ipld, bundle, store_messages, events, waitlist);
      }
    }
  }  // namespace

  BlocksyncClient::BlocksyncClient(
      std::shared_ptr<libp2p::Host> host,
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld)
      : host_(std::move(host)), ipld_(std::move(ipld)) {
    assert(host_);
    assert(ipld_);
  }

  BlocksyncClient::~BlocksyncClient() {
    if (!initialized_) {
      return;
    }
    for (auto &[_, ctx] : requests_) {
      if (ctx.stream) {
        ctx.stream->close();
      }
    }
  }

  void BlocksyncClient::start(std::shared_ptr<events::Events> events) {
    events_ = std::move(events);
    initialized_ = true;
  }

  outcome::result<void> BlocksyncClient::makeRequest(const PeerId &peer,
                                                     std::vector<CID> blocks,
                                                     uint64_t depth,
                                                     RequestOptions options) {
    if (!initialized_) {
      return Error::BLOCKSYNC_CLIENT_NOT_INITALIZED;
    }

    blocks = tryReduceRequest(std::move(blocks), *ipld_, *events_);

    if (blocks.empty()) {
      return outcome::success();
    }

    if (options == MESSAGES_ONLY) {
      // not supported yet
      return Error::BLOCKSYNC_FEATURE_NYI;
    }

    auto counter = ++request_counter_;

    std::unordered_set<CID> waitlist(blocks.begin(), blocks.end());

    requests_.insert({counter, Ctx{std::move(waitlist), options, {}, peer}});

    OUTCOME_TRY(
        binary_request,
        codec::cbor::encode<Request>({std::move(blocks), depth, options}));

    host_->newStream(
        // peer must be already connected
        libp2p::peer::PeerInfo{peer, {}},
        kProtocolId,
        [wptr = weak_from_this(),
         counter,
         binary_request = std::move(binary_request)](auto rstream) mutable {
          auto self = wptr.lock();
          if (self) {
            if (rstream) {
              if (self->initialized_) {
                self->onConnected(
                    counter,
                    std::move(binary_request),
                    std::make_shared<CborStream>(rstream.value()));
              } else {
                rstream.value()->reset();
              }
            } else if (self->initialized_) {
              self->onConnected(counter, common::Buffer{}, rstream.error());
            }
          }
        });

    return outcome::success();
  }

  void BlocksyncClient::onConnected(uint64_t counter,
                                    common::Buffer binary_request,
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

    if (!rstream) {
      e = rstream.error();
    }

    if (!e) {
      ctx.stream = std::move(rstream.value());

      // clang-format off
      ctx.stream->stream()->write(
          binary_request,
          binary_request.size(),
        [wptr = weak_from_this(), buf = binary_request, counter]
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
        rstream.value()->close();
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

    if (!response.chain.empty()) {
      storeResponse(ctx.peer,
                    ipld_,
                    response.chain,
                    ctx.options == BLOCKS_AND_MESSAGES,
                    *events_,
                    ctx.waitlist);
    }

    closeRequest(it, std::error_code{});
  }

  void BlocksyncClient::closeRequest(Requests::iterator it,
                                     std::error_code error) {
    auto ctx = std::move(it->second);
    requests_.erase(it);

    if (ctx.stream) {
      ctx.stream->close();
    }

    if (!ctx.waitlist.empty()) {
      if (!error) {
        error = Error::BLOCKSYNC_INCOMPLETE_RESPONSE;
      } else {
        log()->debug(
            "response error {}, peer {}", error.message(), ctx.peer.toBase58());
      }

      for (auto &cid : ctx.waitlist) {
        events_->signalBlockStored({.from = std::move(ctx.peer),
                                    .block_cid = std::move(cid),
                                    .block = error,
                                    .messages_stored = false});
      }
    }
  }

}  // namespace fc::sync::blocksync

OUTCOME_CPP_DEFINE_CATEGORY(fc::sync::blocksync, BlocksyncClient::Error, e) {
  using E = fc::sync::blocksync::BlocksyncClient::Error;

  switch (e) {
    case E::BLOCKSYNC_CLIENT_NOT_INITALIZED:
      return "blocksync client: not initialized";
    case E::BLOCKSYNC_FEATURE_NYI:
      return "blocksync client: feature NYI";
    case E::BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH:
      return "blocksync client: CIDs mismatch";
    case E::BLOCKSYNC_INCONSISTENT_RESPONSE:
      return "blocksync client: inconsistent response";
    case E::BLOCKSYNC_INCOMPLETE_RESPONSE:
      return "blocksync client: incomplete response";
    default:
      break;
  }
  return "BlocksyncClient::Error: unknown error";
}
