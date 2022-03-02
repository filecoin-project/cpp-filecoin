/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/blocksync_request.hpp"

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/host/host.hpp>
#include <unordered_set>

#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "primitives/tipset/load.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

#define TRACE_ENABLED 0

namespace fc::sync::blocksync {

  using common::libp2p::CborStream;
  using libp2p::basic::Scheduler;

  namespace {
    constexpr size_t kMaxDepth =
        vm::actor::builtin::types::miner::kChainFinality;

    using primitives::block::MsgMeta;

    auto log() {
      static common::Logger logger = common::createLogger("blocksync_client");
      return logger.get();
    }

    template <typename... Args>
    inline void trace(spdlog::string_view_t fmt, const Args &...args) {
#if TRACE_ENABLED
      log()->trace(fmt, args...);
#endif
    }

    std::string statusToString(ResponseStatus status) {
      switch (status) {
#define CASE(Case)           \
  case ResponseStatus::Case: \
    return #Case;

        CASE(kResponseComplete)
        CASE(kResponsePartial)
        CASE(kBlockNotFound)
        CASE(kGoAway)
        CASE(kInternalError)
        CASE(kBadRequest)
#undef CASE

        default:
          break;
      }

      return std::string("unknown status ") + std::to_string(int(status));
    }

    /// Tries to find block in local store
    /// \param cid Block CID
    /// \param ipld Local store
    /// \param require_meta If set when all messages are required too
    /// \return Non-empty object if block found
    boost::optional<BlockHeader> findBlockInLocalStore(const CbCid &cid,
                                                       const IpldPtr &ipld,
                                                       bool require_meta) {
      auto header = getCbor<BlockHeader>(ipld, CID{cid});
      if (!header) {
        return boost::none;
      }

      if (!require_meta) {
        return header.value();
      }

      auto meta = getCbor<MsgMeta>(ipld, header.value().messages);
      if (!meta) {
        return boost::none;
      }

      bool all_messages_available = true;

      auto res = meta.value().bls_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            if (all_messages_available) {
              OUTCOME_TRY(contains, ipld->contains(cid));
              if (!contains) {
                all_messages_available = false;
              }
            }
            return outcome::success();
          });

      if (!res || !all_messages_available) {
        return boost::none;
      }

      res = meta.value().secp_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            if (all_messages_available) {
              OUTCOME_TRY(contains, ipld->contains(cid));
              if (!contains) {
                all_messages_available = false;
              }
            }
            return outcome::success();
          });

      if (!res || !all_messages_available) {
        return boost::none;
      }

      return header.value();
    }

    /// Tries to reduce blocksync request if some data is available locally
    /// \param blocks CIDs of blocks
    /// \param blocks_available (out) blocks found locally are written here
    /// \param ipld Local store
    /// \param require_meta If set when block considered found if all maeeages
    /// are in store
    /// \return Reduced block CIDs
    std::vector<CbCid> tryReduceRequest(
        std::vector<CbCid> blocks,
        std::vector<BlockHeader> &blocks_available,
        const IpldPtr &ipld,
        bool require_meta) {
      std::vector<CbCid> reduced;
      reduced.reserve(blocks.size());
      blocks_available.reserve(blocks.size());
      for (auto &cid : blocks) {
        auto header_found = findBlockInLocalStore(cid, ipld, require_meta);
        if (!header_found) {
          reduced.push_back(std::move(cid));
        } else {
          blocks_available.push_back(std::move(header_found.value()));
        }
      }
      return reduced;
    }

    /// Stores part of blocksync response (i.e. tipset bundle)
    outcome::result<void> storeTipsetBundle(
        IpldPtr ipld,
        std::shared_ptr<PutBlockHeader> put_block_header,
        TipsetBundle &bundle,
        bool store_messages,
        const std::function<void(CbCid, BlockHeader)> &block_stored) {
      size_t sz = bundle.blocks.size();

      auto _msgs{bundle.messages};
      if (_msgs) {
        trace(
            "storing tipset bundle of {} blocks, {} bls messages, {} secp "
            "messages",
            sz,
            _msgs->bls_msgs.size(),
            _msgs->secp_msgs.size());
      } else {
        trace("storing tipset bundle of {} blocks", sz);
      }

      std::vector<CID> secp_cids;
      std::vector<CID> bls_cids;

      try {
        if (_msgs) {
          if (_msgs->secp_msg_includes.size() != sz
              || _msgs->bls_msg_includes.size() != sz) {
            throw std::system_error(
                BlocksyncRequest::Error::kInconsistentResponse);
          }

          secp_cids.reserve(_msgs->secp_msgs.size());
          for (const auto &msg : _msgs->secp_msgs) {
            OUTCOME_EXCEPT(cid, setCbor(ipld, msg));
            secp_cids.push_back(std::move(cid));
          }

          bls_cids.reserve(_msgs->bls_msgs.size());
          for (const auto &msg : _msgs->bls_msgs) {
            OUTCOME_EXCEPT(cid, setCbor(ipld, msg));
            bls_cids.push_back(std::move(cid));
          }
        }
      } catch (const std::system_error &e) {
        log()->error("cannot store tipset bundle, {}", e.code().message());
        return e.code();
      }

      for (size_t i = 0; i < sz; ++i) {
        auto &header{bundle.blocks[i]};
        const auto block_cid{
            primitives::tipset::put(ipld, put_block_header, header)};
        if (_msgs) {
          MsgMeta meta;
          cbor_blake::cbLoadT(ipld, meta);
          for (auto idx : _msgs->secp_msg_includes[i]) {
            if (idx >= secp_cids.size()) {
              return BlocksyncRequest::Error::kInconsistentResponse;
            }
            OUTCOME_TRY(meta.secp_messages.append(secp_cids[idx]));
          }
          for (auto idx : _msgs->bls_msg_includes[i]) {
            if (idx >= bls_cids.size()) {
              return BlocksyncRequest::Error::kInconsistentResponse;
            }
            OUTCOME_TRY(meta.bls_messages.append(bls_cids[idx]));
          }
          OUTCOME_TRY(meta_cid, setCbor(ipld, meta));
          if (meta_cid != header.messages) {
            return BlocksyncRequest::Error::kStoreCidsMismatch;
          }
        }
        block_stored(std::move(*asBlake(block_cid)), std::move(header));
      }

      return outcome::success();
    }

    class BlocksyncRequestImpl
        : public BlocksyncRequest,
          public std::enable_shared_from_this<BlocksyncRequestImpl> {
     public:
      BlocksyncRequestImpl(libp2p::Host &host,
                           Scheduler &scheduler,
                           IpldPtr ipld,
                           std::shared_ptr<PutBlockHeader> put_block_header)
          : host_(host),
            scheduler_(scheduler),
            ipld_(ipld),
            put_block_header_{put_block_header} {}

      ~BlocksyncRequestImpl() override {
        cancel();
      }

      void makeRequest(PeerId peer,
                       std::vector<CbCid> blocks,
                       uint64_t depth,
                       RequestOptions options,
                       uint64_t timeoutMsec,
                       std::function<void(Result)> callback) {
        callback_ = std::move(callback);
        result_.emplace();
        result_->from = peer;
        result_->blocks_requested = std::move(blocks);
        result_->messages_stored = (options & kMessagesOnly);
        result_->messages_only = options == kMessagesOnly;

        std::vector<CbCid> blocks_reduced =
            tryReduceRequest(result_->blocks_requested,
                             result_->blocks_available,
                             ipld_,
                             result_->messages_stored);

        if (blocks_reduced.empty()) {
          scheduleResult();
          return;
        }

        if (result_->messages_only) {
          auto cb{[&](std::error_code ec) {
            result_->error = ec;
            scheduleResult();
          }};
          result_->blocks_available.clear();
          for (const auto &cid : blocks_reduced) {
            OUTCOME_CB(result_->blocks_available.emplace_back(),
                       getCbor<BlockHeader>(ipld_, CID{cid}));
          }
        }

        waitlist_.insert(blocks_reduced.begin(), blocks_reduced.end());

        if (depth == 0) {
          depth = 1;
        } else if (depth > kMaxDepth) {
          depth = kMaxDepth;
        }

        auto maybe_binary_request = codec::cbor::encode<Request>(
            {std::move(blocks_reduced), depth, options});

        if (!maybe_binary_request) {
          result_->error = maybe_binary_request.error();
          scheduleResult();
          return;
        }

        options_ = options;

        host_.newStream(
            // peer must be already connected
            libp2p::peer::PeerInfo{std::move(peer), {}},
            kProtocolId,
            [wptr = weak_from_this(),
             shared_request = std::make_shared<Bytes>(
                 std::move(maybe_binary_request.value()))](auto rstream) {
              auto self = wptr.lock();
              if (self) {
                if (rstream) {
                  self->onConnected(
                      shared_request,
                      std::make_shared<CborStream>(rstream.value()));
                } else {
                  self->onConnected(shared_request, rstream.error());
                }
              }
            });

        if (timeoutMsec > 0) {
          handle_ = scheduler_.scheduleWithHandle(
              [this] {
                result_->error = BlocksyncRequest::Error::kTimeout;
                scheduleResult(true);
              },
              std::chrono::milliseconds(timeoutMsec + depth * 100));
        }
      }

      void cancel() override {
        done();
      }

     private:
      using StreamPtr = std::shared_ptr<CborStream>;

      void done() {
        handle_.cancel();
        in_progress_ = false;
        if (stream_) {
          stream_->close();
        }
      }

      void scheduleResult(bool call_now = false) {
        if (!in_progress_) {
          return;
        }

        done();

        if (result_->error) {
          log()->debug("peer {}, error {}",
                       (result_->from.has_value() ? result_->from->toBase58()
                                                  : "unknown"),
                       result_->error.message());
        }

        if (call_now) {
          callback_(std::move(result_.value()));
        } else {
          handle_ = scheduler_.scheduleWithHandle(
              [this] { callback_(std::move(result_.value())); });
        }
      }

      /**
       * @param binary_request - shared pointer to binary request data that must
       * be alive until libp2p callback in stream::write() is called
       */
      void onConnected(std::shared_ptr<Bytes> binary_request,
                       outcome::result<StreamPtr> rstream) {
        if (!in_progress_) {
          return;
        }

        if (rstream) {
          stream_ = std::move(rstream.value());
          stream_->stream()->write(
              *binary_request,
              binary_request->size(),
              [wptr = weak_from_this(), binary_request](auto res) {
                auto self = wptr.lock();
                if (self) {
                  self->onRequestWritten(res);
                }
              });
        } else {
          result_->error = rstream.error();
          scheduleResult(false);
        }
      }

      void onRequestWritten(outcome::result<size_t> result) {
        if (!in_progress_) {
          return;
        }

        if (!result) {
          result_->error = result.error();
          scheduleResult(true);
          return;
        }

        stream_->read<Response>([wptr = weak_from_this()](auto res) {
          auto self = wptr.lock();
          if (self) {
            self->onResponseRead(std::move(res));
          }
        });
      }

      void onResponseRead(outcome::result<Response> result) {
        if (!in_progress_) {
          return;
        }

        if (!result) {
          log()->debug("error from {}: {}",
                       result_->from->toBase58(),
                       result.error().message());
          result_->error = result.error();
        } else {
          auto &response = result.value();
          log()->debug("got response from {}: status={}, msg=({}), size={}",
                       result_->from->toBase58(),
                       statusToString(response.status),
                       response.message,
                       response.chain.size());
          if (response.chain.size() > 0) {
            storeChain(std::move(response.chain));
          } else {
            result_->error = BlocksyncRequest::Error::kIncompleteResponse;
          }
        }
        scheduleResult(true);
      }

      void storeChain(std::vector<TipsetBundle> chain) {
        auto sz = chain.size();
        if (sz == 0) {
          return;
        }

        boost::optional<TipsetHash> expected_parent;

        if (result_->messages_only) {
          chain[0].blocks = std::move(result_->blocks_available);
        }
        auto res = storeTipsetBundle(
            ipld_,
            put_block_header_,
            chain[0],
            result_->messages_stored,
            [&, this](CbCid cid, BlockHeader header) {
              if (auto it = waitlist_.find(cid); it != waitlist_.end()) {
                waitlist_.erase(it);
                if (sz > 1 && !expected_parent) {
                  expected_parent = TipsetKey::hash(header.parents);
                }
                result_->blocks_available.push_back(std::move(header));
              }
            });

        if (!res) {
          log()->error("store tipset bundle error, {}", res.error().message());
          result_->error = res.error();
        } else if (!waitlist_.empty()) {
          log()->debug("got incomplete response, got {} of {}",
                       result_->blocks_available.size(),
                       result_->blocks_requested.size());
          result_->error = Error::kIncompleteResponse;
        }

        if (sz == 1 || !expected_parent) {
          return;
        }

        result_->parents.reserve(sz - 1);

        primitives::tipset::TipsetCreator creator;

        for (size_t i = 1; i < sz; ++i) {
          res = storeTipsetBundle(
              ipld_,
              put_block_header_,
              chain[i],
              result_->messages_stored,
              [&](CbCid cid, BlockHeader header) {
                if (expected_parent) {
                  if (auto r = creator.canExpandTipset(header); !r) {
                    log()->warn("cannot expand tipset, {}",
                                r.error().message());
                    result_->error = Error::kInconsistentResponse;
                    expected_parent.reset();
                  } else if (auto r1 = creator.expandTipset(std::move(cid),
                                                            std::move(header));
                             !r1) {
                    log()->warn("cannot expand tipset, {}",
                                r1.error().message());
                    result_->error = Error::kInconsistentResponse;
                    expected_parent.reset();
                  }
                }
              });

          if (res) {
            if (expected_parent) {
              auto tipset = creator.getTipset(true);
              if (tipset->key.hash() != expected_parent.value()) {
                log()->warn("unexpected parent returned");
                // dont try to save parents anymore
                expected_parent.reset();
                result_->error = Error::kInconsistentResponse;
              } else {
                expected_parent = tipset->getParents().hash();
                result_->parents.push_back(std::move(tipset));
              }
            }
          } else {
            log()->error("store tipset bundle error, {}",
                         res.error().message());
            // dont try to save parents anymore
            break;
          }

          if (!expected_parent) {
            break;
          }
        }
      }

      libp2p::Host &host_;
      Scheduler &scheduler_;
      IpldPtr ipld_;
      std::shared_ptr<PutBlockHeader> put_block_header_;
      std::function<void(Result)> callback_;
      boost::optional<Result> result_;
      RequestOptions options_ = kBlocksAndMessages;
      std::unordered_set<CbCid> waitlist_;
      Scheduler::Handle handle_;
      StreamPtr stream_;
      bool in_progress_ = true;
    };

  }  // namespace

  std::shared_ptr<BlocksyncRequest> BlocksyncRequest::newRequest(
      libp2p::Host &host,
      libp2p::basic::Scheduler &scheduler,
      IpldPtr ipld,
      std::shared_ptr<PutBlockHeader> put_block_header,
      PeerId peer,
      std::vector<CbCid> blocks,
      uint64_t depth,
      RequestOptions options,
      uint64_t timeoutMsec,
      std::function<void(Result)> callback) {
    assert(callback);

    std::shared_ptr<BlocksyncRequestImpl> impl =
        std::make_shared<BlocksyncRequestImpl>(
            host, scheduler, ipld, put_block_header);

    // need shared_from_this there
    impl->makeRequest(std::move(peer),
                      std::move(blocks),
                      depth,
                      options,
                      timeoutMsec,
                      std::move(callback));

    return impl;
  }

}  // namespace fc::sync::blocksync

OUTCOME_CPP_DEFINE_CATEGORY(fc::sync::blocksync, BlocksyncRequest::Error, e) {
  using E = fc::sync::blocksync::BlocksyncRequest::Error;

  switch (e) {
    case E::kNotImplemented:
      return "blocksync client: feature is not yet implemented";
    case E::kStoreCidsMismatch:
      return "blocksync client: CIDs mismatch";
    case E::kInconsistentResponse:
      return "blocksync client: inconsistent response";
    case E::kIncompleteResponse:
      return "blocksync client: incomplete response";
    case E::kTimeout:
      return "blocksync client: timeout";
    default:
      break;
  }
  return "BlocksyncRequest::Error: unknown error";
}
