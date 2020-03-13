/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_API_HPP
#define CPP_FILECOIN_CORE_API_API_HPP

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/ticket/epost_ticket.hpp"
#include "primitives/ticket/ticket.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"
#include "vm/runtime/runtime_types.hpp"

#define API_METHOD(_name, _result, ...)                                    \
  struct _##_name : std::function<outcome::result<_result>(__VA_ARGS__)> { \
    using Result = _result;                                                \
    using Params = ParamsTuple<__VA_ARGS__>;                               \
    static constexpr auto name = "Filecoin." #_name;                       \
  } _name;

namespace fc::api {
  using common::Buffer;
  using common::Comm;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StoragePower;
  using primitives::TipsetWeight;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::block::Block;
  using primitives::ticket::EPostProof;
  using primitives::ticket::Ticket;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using vm::actor::Actor;
  using vm::actor::builtin::market::OnChainDeal;
  using vm::actor::builtin::market::StorageParticipantBalance;
  using vm::actor::builtin::payment_channel::SignedVoucher;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::ExecutionResult;
  using vm::runtime::MessageReceipt;

  template <typename... T>
  using ParamsTuple =
      std::tuple<std::remove_const_t<std::remove_reference_t<T>>...>;

  template <typename T>
  struct Chan {
    uint64_t id;
  };

  struct HeadChange {
    enum class Type { Current, Revert, Apply };

    Type type;
    Tipset tipset;
  };

  struct InvocResult {
    UnsignedMessage message;
    MessageReceipt receipt;
    std::vector<ExecutionResult> internal_executions;
    std::string error;
  };

  using OnChainDealMap = std::map<std::string, OnChainDeal>;

  struct MinerPower {
    StoragePower miner;
    StoragePower total;
  };

  struct ChainSectorInfo {
    SectorNumber sector;
    Comm comm_d;
    Comm comm_r;
  };

  struct MsgWait {
    MessageReceipt receipt;
    Tipset tipset;
  };

  struct Api {
    API_METHOD(ChainGetRandomness, Randomness, const TipsetKey &, int64_t)
    API_METHOD(ChainHead, Tipset)
    API_METHOD(ChainNotify, Chan<HeadChange>)
    API_METHOD(ChainReadObj, Buffer, CID)
    API_METHOD(ChainTipSetWight, TipsetWeight, const TipsetKey &)

    API_METHOD(MarketEnsureAvailable, void, const Address &, TokenAmount)

    API_METHOD(MinerCreateBlock,
               Block,
               const Address &,
               const TipsetKey &,
               const Ticket &,
               const EPostProof &,
               const std::vector<SignedMessage> &,
               uint64_t,
               uint64_t)

    API_METHOD(MpoolPending, std::vector<SignedMessage>, const TipsetKey &)
    API_METHOD(MpoolPushMessage, SignedMessage, const UnsignedMessage &)

    API_METHOD(PaychVoucherAdd,
               TokenAmount,
               const Address &,
               const SignedVoucher &,
               const Buffer &,
               TokenAmount)

    API_METHOD(StateCall,
               InvocResult,
               const UnsignedMessage &,
               const TipsetKey &)
    API_METHOD(StateGetActor, Actor, const Address &, const TipsetKey &)
    API_METHOD(StateMarketBalance,
               StorageParticipantBalance,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMarketDeals, OnChainDealMap, const TipsetKey &)
    API_METHOD(StateMarketStorageDeal, OnChainDeal, DealId, const TipsetKey &)
    API_METHOD(StateMinerElectionPeriodStart,
               ChainEpoch,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerFaults, RleBitset, const Address &, const TipsetKey &)
    API_METHOD(StateMinerPower, MinerPower, const Address &, const TipsetKey &)
    API_METHOD(StateMinerProvingSet,
               std::vector<ChainSectorInfo>,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerSectorSize,
               SectorSize,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerWorker, Address, const Address &, const TipsetKey &)
    API_METHOD(StateWaitMsg, MsgWait, const CID &)

    API_METHOD(SyncSubmitBlock, void, const Block &)

    API_METHOD(WalletSign, Signature, const Address &, const Buffer &)
  };
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_API_HPP
