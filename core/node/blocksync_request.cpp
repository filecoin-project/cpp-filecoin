/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unordered_set>

#include "blocksync_request.hpp"

#include <libp2p/host/host.hpp>

#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "storage/ipfs/datastore.hpp"

#define TRACE_ENABLED 0

namespace fc::sync::blocksync {

  using common::libp2p::CborStream;

  namespace {
    constexpr size_t kMaxDepth = 100;

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

    std::string statusToString(ResponseStatus status) {
      switch (status) {
#define CASE(Case)           \
  case ResponseStatus::Case: \
    return #Case;

        CASE(RESPONSE_COMPLETE)
        CASE(RESPONSE_PARTIAL)
        CASE(BLOCK_NOT_FOUND)
        CASE(GO_AWAY)
        CASE(INTERNAL_ERROR)
        CASE(BAD_REQUEST)
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
    boost::optional<BlockHeader> findBlockInLocalStore(const CID &cid,
                                                       Ipld &ipld,
                                                       bool require_meta) {
      auto header = ipld.getCbor<BlockHeader>(cid);
      if (!header) {
        return boost::none;
      }

      if (!require_meta) {
        return header.value();
      }

      auto meta = ipld.getCbor<MsgMeta>(header.value().messages);
      if (!meta) {
        return boost::none;
      }

      bool all_messages_available = true;

      auto res = meta.value().bls_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            if (all_messages_available) {
              OUTCOME_TRY(contains, ipld.contains(cid));
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
              OUTCOME_TRY(contains, ipld.contains(cid));
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
    std::vector<CID> tryReduceRequest(
        std::vector<CID> blocks,
        std::vector<BlockHeader> &blocks_available,
        storage::ipfs::IpfsDatastore &ipld,
        bool require_meta) {
      std::vector<CID> reduced;
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
        Ipld &ipld,
        TipsetBundle &bundle,
        bool store_messages,
        const std::function<void(CID, BlockHeader)> &block_stored) {
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
                BlocksyncRequest::Error::BLOCKSYNC_INCONSISTENT_RESPONSE);
          }

          secp_cids.reserve(_msgs->secp_msgs.size());
          for (const auto &msg : _msgs->secp_msgs) {
            OUTCOME_EXCEPT(cid,
                           ipld.setCbor<primitives::block::SignedMessage>(msg));
            secp_cids.push_back(std::move(cid));
          }

          bls_cids.reserve(_msgs->bls_msgs.size());
          for (const auto &msg : _msgs->bls_msgs) {
            OUTCOME_EXCEPT(cid, ipld.setCbor<UnsignedMessage>(msg));
            bls_cids.push_back(std::move(cid));
          }
        }
      } catch (const std::system_error &e) {
        log()->error("cannot store tipset bundle, {}", e.code().message());
        return e.code();
      }

      for (size_t i = 0; i < sz; ++i) {
        auto &header{bundle.blocks[i]};
        OUTCOME_TRY(block_cid, ipld.setCbor<BlockHeader>(header));
        if (_msgs) {
          MsgMeta meta;
          ipld.load(meta);
          for (auto idx : _msgs->secp_msg_includes[i]) {
            if (idx >= secp_cids.size()) {
              return BlocksyncRequest::Error::BLOCKSYNC_INCONSISTENT_RESPONSE;
            }
            OUTCOME_TRY(meta.secp_messages.append(secp_cids[idx]));
          }
          for (auto idx : _msgs->bls_msg_includes[i]) {
            if (idx >= bls_cids.size()) {
              return BlocksyncRequest::Error::BLOCKSYNC_INCONSISTENT_RESPONSE;
            }
            OUTCOME_TRY(meta.bls_messages.append(bls_cids[idx]));
          }
          OUTCOME_TRY(meta_cid, ipld.setCbor<MsgMeta>(meta));
          if (meta_cid != header.messages) {
            return BlocksyncRequest::Error::BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH;
          }
          block_stored(std::move(block_cid), std::move(header));
        }
      }

      return outcome::success();
    }

    // XXX
    static int xxx = 0;

    class BlocksyncRequestImpl
        : public BlocksyncRequest,
          public std::enable_shared_from_this<BlocksyncRequestImpl> {
     public:
      BlocksyncRequestImpl(libp2p::Host &host,
                           libp2p::protocol::Scheduler &scheduler,
                           Ipld &ipld)
          : host_(host), scheduler_(scheduler), ipld_(ipld) {
        log()->debug("++++++ {}", ++xxx);
      }

      ~BlocksyncRequestImpl() {
        log()->debug("------ {}", --xxx);
        cancel();
      }

      void makeRequest(PeerId peer,
                       std::vector<CID> blocks,
                       uint64_t depth,
                       RequestOptions options,
                       uint64_t timeoutMsec,
                       std::function<void(Result)> callback) {
        callback_ = std::move(callback);
        result_.emplace();
        result_->blocks_requested = std::move(blocks);
        result_->messages_stored = (options & MESSAGES_ONLY);

        std::vector<CID> blocks_reduced =
            tryReduceRequest(result_->blocks_requested,
                             result_->blocks_available,
                             ipld_,
                             result_->messages_stored);

        if (blocks_reduced.empty()) {
          scheduleResult();
          return;
        }

        if (options == MESSAGES_ONLY) {
          // not supported yet
          result_->error = BlocksyncRequest::Error::BLOCKSYNC_FEATURE_NYI;
          scheduleResult();
          return;
        }

        waitlist_.insert(blocks_reduced.begin(), blocks_reduced.end());

        if (depth == 0) {
          depth = 1;
        } else if (depth > kMaxDepth) {
          depth = kMaxDepth;
        }

        auto binary_request = codec::cbor::encode<Request>(
            {std::move(blocks_reduced), depth, options});

        if (!binary_request) {
          result_->error = binary_request.error();
          scheduleResult();
          return;
        }

        options_ = options;
        result_->from = peer;

        host_.newStream(
            // peer must be already connected
            libp2p::peer::PeerInfo{std::move(peer), {}},
            kProtocolId,
            [wptr = weak_from_this(),
             binary_request = std::move(binary_request.value())](auto rstream) {
              auto self = wptr.lock();
              if (self) {
                if (rstream) {
                  self->onConnected(
                      std::move(binary_request),
                      std::make_shared<CborStream>(rstream.value()));
                } else {
                  self->onConnected(common::Buffer{}, rstream.error());
                }
              }
            });

        if (timeoutMsec > 0) {
          handle_ = scheduler_.schedule(timeoutMsec + depth * 100, [this] {
            result_->error = BlocksyncRequest::Error::BLOCKSYNC_TIMEOUT;
            scheduleResult(true);
          });
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
          int64_t dr = 0;
          auto &category =
              std::error_code(BlocksyncRequest::Error::BLOCKSYNC_TIMEOUT)
                  .category();
          if (result_->error.category() == category) {
            switch (result_->error.value()) {
              case int(
                  BlocksyncRequest::Error::BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH):
                dr = -700;
                break;
              case int(
                  BlocksyncRequest::Error::BLOCKSYNC_INCONSISTENT_RESPONSE):
                dr = -500;
                break;
              case int(BlocksyncRequest::Error::BLOCKSYNC_TIMEOUT):
                dr = -200;
                break;
              default:
                break;
            }
          } else {
            // stream and other errors
            dr -= 200;
          }
          log()->debug("peer {}, error {}, dr={}",
                       (result_->from.has_value() ? result_->from->toBase58()
                                                  : "unknown"),
                       result_->error.message(),
                       dr);
          result_->delta_rating += dr;
        }

        if (call_now) {
          callback_(std::move(result_.value()));
        } else {
          handle_ = scheduler_.schedule(
              [this] { callback_(std::move(result_.value())); });
        }
      }

      void onConnected(common::Buffer binary_request,
                       outcome::result<StreamPtr> rstream) {
        if (!in_progress_) {
          return;
        }

        if (rstream) {
          stream_ = std::move(rstream.value());
          stream_->stream()->write(
              binary_request,
              binary_request.size(),
              [wptr = weak_from_this(), buf = binary_request](auto res) {
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

          if (response.status == ResponseStatus::RESPONSE_COMPLETE) {
            result_->delta_rating += 100;
          }
          if (response.chain.size() > 0) {
            result_->delta_rating += 50;
            storeChain(std::move(response.chain));
          } else {
            result_->delta_rating -= 50;
            result_->error =
                BlocksyncRequest::Error::BLOCKSYNC_INCOMPLETE_RESPONSE;
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

        auto res = storeTipsetBundle(
            ipld_,
            chain[0],
            result_->messages_stored,
            [&, this](CID cid, BlockHeader header) {
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
          result_->error = Error::BLOCKSYNC_INCOMPLETE_RESPONSE;
        }

        if (sz == 1 || !expected_parent) {
          return;
        }

        result_->parents.reserve(sz - 1);

        primitives::tipset::TipsetCreator creator;

        for (size_t i = 1; i < sz; ++i) {
          res = storeTipsetBundle(
              ipld_,
              chain[i],
              result_->messages_stored,
              [&](CID cid, BlockHeader header) {
                if (expected_parent) {
                  if (auto r = creator.canExpandTipset(header); !r) {
                    log()->warn("cannot expand tipset, {}",
                                r.error().message());
                    result_->error = Error::BLOCKSYNC_INCONSISTENT_RESPONSE;
                    expected_parent.reset();
                  } else if (auto r1 = creator.expandTipset(std::move(cid),
                                                            std::move(header));
                             !r1) {
                    log()->warn("cannot expand tipset, {}",
                                r1.error().message());
                    result_->error = Error::BLOCKSYNC_INCONSISTENT_RESPONSE;
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
                result_->error = Error::BLOCKSYNC_INCONSISTENT_RESPONSE;
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

        if (!result_->blocks_available.empty()) {
          result_->delta_rating +=
              (result_->blocks_available.size() + result_->parents.size()) * 5;
        }
      }

      libp2p::Host &host_;
      libp2p::protocol::Scheduler &scheduler_;
      Ipld &ipld_;
      std::function<void(Result)> callback_;
      boost::optional<Result> result_;
      RequestOptions options_ = BLOCKS_AND_MESSAGES;
      std::unordered_set<CID> waitlist_;
      libp2p::protocol::scheduler::Handle handle_;
      StreamPtr stream_;
      bool in_progress_ = true;
    };

  }  // namespace

  std::shared_ptr<BlocksyncRequest> BlocksyncRequest::newRequest(
      libp2p::Host &host,
      libp2p::protocol::Scheduler &scheduler,
      Ipld &ipld,
      PeerId peer,
      std::vector<CID> blocks,
      uint64_t depth,
      RequestOptions options,
      uint64_t timeoutMsec,
      std::function<void(Result)> callback) {
    assert(callback);

    std::shared_ptr<BlocksyncRequestImpl> impl =
        std::make_shared<BlocksyncRequestImpl>(host, scheduler, ipld);

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
    case E::BLOCKSYNC_FEATURE_NYI:
      return "blocksync client: feature NYI";
    case E::BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH:
      return "blocksync client: CIDs mismatch";
    case E::BLOCKSYNC_INCONSISTENT_RESPONSE:
      return "blocksync client: inconsistent response";
    case E::BLOCKSYNC_INCOMPLETE_RESPONSE:
      return "blocksync client: incomplete response";
    case E::BLOCKSYNC_TIMEOUT:
      return "blocksync client: timeout";
    default:
      break;
  }
  return "BlocksyncRequest::Error: unknown error";
}
