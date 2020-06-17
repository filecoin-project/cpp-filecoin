/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/events/impl/events_impl.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc::markets::storage::events {
  using primitives::tipset::HeadChangeType;
  using vm::actor::builtin::miner::PreCommitSector;
  using vm::actor::builtin::miner::ProveCommitSector;
  using vm::actor::builtin::miner::SectorPreCommitInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using PromiseResult = Events::PromiseResult;

  EventsImpl::EventsImpl(std::shared_ptr<Api> api, IpldPtr ipld)
      : api_{std::move(api)}, ipld_{std::move(ipld)} {}

  outcome::result<void> EventsImpl::init() {
    OUTCOME_TRY(chan, api_->ChainNotify());
    chan.channel->read(
        [self{shared_from_this()}, channel{chan.channel}](
            boost::optional<std::vector<HeadChange>> update) -> bool {
          return self->onRead(update);
        });
    return outcome::success();
  }

  std::shared_ptr<PromiseResult> EventsImpl::onDealSectorCommitted(
      const Address &provider, const DealId &deal_id) {
    auto result = std::make_shared<PromiseResult>();
    std::unique_lock lock(watched_events_mutex_);
    watched_events_.emplace_back(EventWatch{.provider = provider,
                                            .deal_id = deal_id,
                                            .sector_number = boost::none,
                                            .result = result});
    return result;
  }

  /**
   * Actually sector commit consists of 2 method calls:
   *  1) PreCommitSector with desired provider address and deal id. Parameters
   * contain sector number used in the next call
   *  2) ProveCommitSector with desired provider address and sector number
   */
  bool EventsImpl::onRead(
      const boost::optional<std::vector<HeadChange>> &changes) {
    if (changes) {
      for (const auto &change : changes.get()) {
        if (change.type == HeadChangeType::APPLY) {
          auto visited = change.value.visitMessages(
              ipld_, [this](auto, auto bls, auto cid) -> outcome::result<void> {
                return onMessage(bls, cid);
              });
          if (visited.has_error()) {
            logger_->error("Visit message error: " + visited.error().message());
          }
        }
      }
    }
    return true;
  };

  outcome::result<void> EventsImpl::onMessage(bool bls,
                                              const CID &message_cid) {
    UnsignedMessage message;
    if (bls) {
      OUTCOME_TRYA(message, ipld_->getCbor<UnsignedMessage>(message_cid));
    } else {
      OUTCOME_TRY(signed_message, ipld_->getCbor<SignedMessage>(message_cid));
      message = std::move(signed_message.message);
    }

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
            // save sector number if deal id matches
            if (std::find(deal_ids.begin(), deal_ids.end(), watch_it->deal_id)
                != deal_ids.end()) {
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
        watch_it->result->set_value(outcome::success());
        watched_events_.erase(watch_it);
      }
    }
    return outcome::success();
  }

}  // namespace fc::markets::storage::events
