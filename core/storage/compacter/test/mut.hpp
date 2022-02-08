#pragma once

#include "common/logger.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "vm/actor/builtin/states/account/account_actor_state.hpp"
#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/codes.hpp"
#include "vm/state/state_tree.hpp"

namespace fc {
  template <typename T>
  auto O(outcome::result<T> &&o) {
    return std::move(o).value();
  }
}  // namespace fc

namespace fc::mut {
  using vm::actor::kInitAddress;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::states::AccountActorStatePtr;
  using vm::actor::builtin::states::InitActorStatePtr;
  using vm::actor::builtin::states::MarketActorStatePtr;
  using vm::actor::builtin::states::PowerActorStatePtr;
  using vm::actor::builtin::types::Universal;
  using vm::actor::builtin::types::market::DealProposal;
  using vm::actor::builtin::types::market::PendingProposals;
  using vm::actor::builtin::types::storage_power::CronEvent;
  using vm::state::Actor;
  using vm::state::Address;
  using vm::state::StateTree;
  using PendingProposalsPtr = Universal<PendingProposals>;
  namespace code = vm::actor::builtin::v7;

  const auto account_actor_address{Address::makeFromId(100)};

  template <typename T>
  struct _IsUniversal;
  template <typename T>
  struct _IsUniversal<Universal<T>> {
    using type = T;
  };
  template <typename T>
  auto _universal(const IpldPtr &ipld) {
    Universal<typename _IsUniversal<T>::type> u{ipld->actor_version};
    cbor_blake::cbLoadT(ipld, u);
    return u;
  }
#define MAKE(T) _universal<T>(ipld)
#define PUT(x) O(setCbor(ipld, x))
#define GET(c, T) O(getCbor<T>(ipld, c))

  // Sets up genesis state
  inline void genesis(const IpldPtr &ipld,
                      const std::shared_ptr<StateTree> &tree) {
    Actor account_actor;
    account_actor.code = code::kAccountCodeId;
    account_actor.head = PUT(MAKE(AccountActorStatePtr));
    O(tree->set(account_actor_address, account_actor));

    Actor init_actor;
    init_actor.code = code::kInitCodeId;
    init_actor.head = PUT(MAKE(InitActorStatePtr));
    O(tree->set(kInitAddress, init_actor));

    Actor market_actor;
    market_actor.code = code::kStorageMarketCodeId;
    MarketActorStatePtr market_actor_state{ipld->actor_version};
    market_actor_state->pending_proposals = MAKE(PendingProposalsPtr);
    cbor_blake::cbLoadT(ipld, market_actor_state);
    market_actor.head = PUT(market_actor_state);
    O(tree->set(kStorageMarketAddress, market_actor));

    Actor power_actor;
    power_actor.code = code::kStoragePowerCodeId;
    power_actor.head = PUT(MAKE(PowerActorStatePtr));
    O(tree->set(kStoragePowerAddress, power_actor));

    spdlog::info("mut::genesis");
  }

  inline void mutateMarketActor(const IpldPtr &ipld,
                                  uint64_t seed,
                                  MarketActorStatePtr &state) {
    CID cid(CID::Version::V1,
            CID::Multicodec::RAW,
            crypto::Hasher::blake2b_256(
                Bytes{32, static_cast<unsigned char>(seed)}));
    Address address = Address::makeFromId(seed);
    auto epoch = gsl::narrow<ChainEpoch>(seed);

    DealProposal proposal{
        .piece_cid = cid,
        .piece_size = primitives::piece::PaddedPieceSize{seed},
        .verified = false,
        .client = address,
        .provider = address,
        .label = std::to_string(seed),
        .start_epoch = epoch,
        .end_epoch = epoch,
        .storage_price_per_epoch = seed,
        .provider_collateral = seed,
        .client_collateral = seed,
    };
    O(state->proposals.set(seed, proposal));

    O(state->states.append({
        .sector_start_epoch = epoch,
        .last_updated_epoch = epoch,
        .slash_epoch = epoch,
    }));

    O(state->pending_proposals->set(cid, proposal));

    O(state->escrow_table.addCreate(address, seed));
    O(state->locked_table.addCreate(address, seed));

    state->next_deal = seed;

    vm::actor::builtin::states::DealSet deal_set{ipld};
    O(deal_set.set(1));
    O(deal_set.set(seed));
    O(state->deals_by_epoch.set(epoch, deal_set));

    state->last_cron = epoch;
    state->total_client_locked_collateral = seed;
    state->total_provider_locked_collateral = seed;
    state->total_client_storage_fee = seed;
  }

  inline void mutatePowerActor(const IpldPtr &ipld,
                                 uint64_t seed,
                                 PowerActorStatePtr &state) {
    Address address = Address::makeFromId(seed);
    auto epoch = gsl::narrow<ChainEpoch>(seed);

    state->total_raw_power = seed;
    state->total_raw_commited = seed;
    state->total_qa_power = seed;
    state->total_qa_commited = seed;
    state->total_pledge_collateral = seed;
    state->this_epoch_raw_power = seed;
    state->this_epoch_qa_power = seed;
    state->this_epoch_pledge_collateral = seed;
    state->this_epoch_qa_power_smoothed = {
        .position = seed,
        .velocity = seed,
    };
    state->miner_count = seed;
    state->num_miners_meeting_min_power = seed;

    CronEvent event{
        .miner_address = address,
        .callback_payload = Bytes{16, static_cast<unsigned char>(seed)},
    };
    adt::Array<CronEvent, 6> events{ipld};
    O(state->cron_event_queue.set(epoch, events));

    state->first_cron_epoch = epoch;
    state->last_processed_cron_epoch = epoch;

    Universal<vm::actor::builtin::types::storage_power::Claim> claim{
        ipld->actor_version};
    claim->raw_power = seed;
    claim->qa_power = seed;
    O(state->claims.set(address, claim));
  }

  // Changes state for new blocks
  inline void block(const IpldPtr &ipld,
                    const std::shared_ptr<StateTree> &tree,
                    ChainEpoch epoch) {
    auto account_actor{O(tree->get(account_actor_address))};
    auto account_actor_state{GET(account_actor.head, AccountActorStatePtr)};
    account_actor_state->address =
        Address::makeFromId(account_actor_state->address.getId() + 1);
    account_actor.head = PUT(account_actor_state);
    O(tree->set(account_actor_address, account_actor));

    auto init_actor{O(tree->get(kInitAddress))};
    auto init_actor_state{GET(init_actor.head, InitActorStatePtr)};
    auto id = init_actor_state->next_id++;
    O(init_actor_state->address_map.set(Address::makeFromId(id), id));
    init_actor.head = PUT(init_actor_state);
    O(tree->set(kInitAddress, init_actor));

    auto market_actor{O(tree->get(kStorageMarketAddress))};
    auto market_actor_state{GET(market_actor.head, MarketActorStatePtr)};
    mutateMarketActor(ipld, epoch, market_actor_state);
    market_actor.head = PUT(market_actor_state);
    O(tree->set(kStorageMarketAddress, market_actor));

    auto power_actor{O(tree->get(kStoragePowerAddress))};
    auto power_actor_state{GET(power_actor.head, PowerActorStatePtr)};
    mutatePowerActor(ipld, epoch, power_actor_state);
    power_actor.head = PUT(power_actor_state);
    O(tree->set(kStoragePowerAddress, power_actor));

    spdlog::info("mut::block {}", epoch);
  }
}  // namespace fc::mut
