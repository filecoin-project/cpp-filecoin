/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::markets::storage::chain_events {
  using primitives::tipset::HeadChangeType;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using vm::actor::builtin::v0::miner::PreCommitSector;
  using vm::actor::builtin::v0::miner::ProveCommitSector;
  using vm::message::SignedMessage;

  ChainEventsImpl::ChainEventsImpl(std::shared_ptr<FullNodeApi> api)
      : api_{std::move(api)} {}

  outcome::result<void> ChainEventsImpl::init() {
    OUTCOME_TRY(chan, api_->ChainNotify());
    channel_ = chan.channel;
    channel_->read(
        [self_weak{weak_from_this()}](
            const boost::optional<std::vector<HeadChange>> &update) -> bool {
          if (auto self = self_weak.lock()) return self->onRead(update);
          return false;
        });
    return outcome::success();
  }

  void ChainEventsImpl::onDealSectorCommitted(const Address &provider,
                                              const DealId &deal_id,
                                              Cb cb) {
    std::unique_lock lock(watched_events_mutex_);
    watched_events_.emplace_back(EventWatch{.provider = provider,
                                            .deal_id = deal_id,
                                            .sector_number = boost::none,
                                            .cb = std::move(cb)});
  }

  /**
   * Actually sector commit consists of 2 method calls:
   *  1) PreCommitSector with desired provider address and deal id. Parameters
   * contain sector number used in the next call
   *  2) ProveCommitSector with desired provider address and sector number
   */
  bool ChainEventsImpl::onRead(
      const boost::optional<std::vector<HeadChange>> &changes) {
    if (changes) {
      for (const auto &change : changes.get()) {
        if (change.type == HeadChangeType::APPLY) {
          for (auto &block_cid : change.value->key.cids()) {
            auto block_messages = api_->ChainGetBlockMessages(CID{block_cid});
            if (block_messages.has_error()) {
              logger_->error("ChainGetBlockMessages error: "
                             + block_messages.error().message());
              continue;
            }
            for (const auto &message : block_messages.value().bls) {
              auto message_processed = onMessage(message, message.getCid());
              if (message_processed.has_error()) {
                logger_->error("Message process error: "
                               + message_processed.error().message());
              }
            }
            for (const auto &message : block_messages.value().secp) {
              auto message_processed =
                  onMessage(message.message, message.getCid());
              if (message_processed.has_error()) {
                logger_->error("Message process error: "
                               + message_processed.error().message());
              }
            }
          }
        }
      }
    }
    return true;
  };

  outcome::result<void> ChainEventsImpl::onMessage(
      const UnsignedMessage &message, const CID &cid) {
    std::vector<EventWatch>::iterator watch_it;
    boost::optional<SectorNumber> update_sector_number;
    bool prove_sector_committed = false;
    {
      std::shared_lock lock(watched_events_mutex_);
      for (watch_it = watched_events_.begin();
           watch_it != watched_events_.end();
           ++watch_it) {
        if (message.to == watch_it->provider) {
          if (message.method == PreCommitSector::Number) {
            OUTCOME_TRY(
                pre_commit_info,
                codec::cbor::decode<SectorPreCommitInfo>(message.params));
            auto deal_ids = pre_commit_info.deal_ids;
            // save sector number if deal id matches and it is not saved yet
            if (std::find(deal_ids.begin(), deal_ids.end(), watch_it->deal_id)
                    != deal_ids.end()
                && !watch_it->sector_number) {
              update_sector_number = pre_commit_info.sector;
              break;
            }
          } else if (message.method == ProveCommitSector::Number) {
            OUTCOME_TRY(
                prove_commit_params,
                codec::cbor::decode<ProveCommitSector::Params>(message.params));
            // notify if sector number matches,
            if (watch_it->sector_number
                && prove_commit_params.sector == watch_it->sector_number) {
              prove_sector_committed = true;
              break;
            }
          }
        }
      }
    }

    if (watch_it != watched_events_.end()) {
      std::unique_lock lock(watched_events_mutex_);
      if (update_sector_number) {
        watch_it->sector_number = update_sector_number;
      }

      if (prove_sector_committed) {
        OUTCOME_TRY(wait, api_->StateWaitMsg(cid, api::kNoConfidence));
        wait.waitOwn([cb{std::move(watch_it->cb)}](auto _r) {
          if (_r) {
            cb();
          }
        });
        watched_events_.erase(watch_it);
      }
    }
    return outcome::success();
  }

}  // namespace fc::markets::storage::chain_events
