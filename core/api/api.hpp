/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_API_HPP
#define CPP_FILECOIN_CORE_API_API_HPP

#include <condition_variable>

#include <libp2p/peer/peer_info.hpp>

#include "adt/channel.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/types.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/ticket/epost_ticket.hpp"
#include "primitives/ticket/ticket.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"
#include "vm/runtime/runtime_types.hpp"

#define API_METHOD(_name, _result, ...)                                    \
  struct _##_name : std::function<outcome::result<_result>(__VA_ARGS__)> { \
    using Result = _result;                                                \
    using Params = ParamsTuple<__VA_ARGS__>;                               \
    static constexpr auto name = "Filecoin." #_name;                       \
  } _name;

namespace fc::api {
  using adt::Channel;
  using common::Buffer;
  using common::Comm;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using libp2p::peer::PeerInfo;
  using markets::storage::SignedStorageAsk;
  using markets::storage::StorageDeal;
  using markets::storage::StorageProviderInfo;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StoragePower;
  using primitives::TipsetWeight;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::block::BlockMsg;
  using primitives::ticket::EPostProof;
  using primitives::ticket::Ticket;
  using primitives::tipset::HeadChange;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using vm::actor::Actor;
  using vm::actor::builtin::market::ClientDealProposal;
  using vm::actor::builtin::market::DealProposal;
  using vm::actor::builtin::market::DealState;
  using vm::actor::builtin::market::StorageParticipantBalance;
  using vm::actor::builtin::miner::MinerInfo;
  using vm::actor::builtin::miner::PoStState;
  using vm::actor::builtin::miner::SectorOnChainInfo;
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
    using Type = T;
    Chan(std::shared_ptr<Channel<T>> channel) : channel{std::move(channel)} {}
    uint64_t id{};
    std::shared_ptr<Channel<T>> channel;
  };

  template <typename T>
  struct is_chan : std::false_type {};

  template <typename T>
  struct is_chan<Chan<T>> : std::true_type {};

  template <typename T>
  struct Wait {
    using Type = T;
    using Result = outcome::result<T>;

    Wait(std::shared_ptr<Channel<Result>> channel)
        : channel{std::move(channel)} {}

    void wait(std::function<void(Result)> cb) {
      channel->read([cb{std::move(cb)}](auto opt) {
        assert(opt);
        cb(std::move(*opt));
        return false;
      });
    }

    auto waitSync() {
      std::condition_variable c;
      Result r{outcome::success()};
      wait([&](auto v) {
        r = v;
        c.notify_one();
      });
      std::mutex m;
      auto l = std::unique_lock{m};
      c.wait(l);
      return r;
    }

    std::shared_ptr<Channel<Result>> channel;
  };

  template <typename T>
  struct is_wait : std::false_type {};

  template <typename T>
  struct is_wait<Wait<T>> : std::true_type {};

  struct InvocResult {
    UnsignedMessage message;
    MessageReceipt receipt;
    std::vector<ExecutionResult> internal_executions;
    std::string error;
  };

  using MarketDealMap = std::map<std::string, StorageDeal>;

  struct MinerPower {
    StoragePower miner;
    StoragePower total;
  };

  struct ChainSectorInfo {
    SectorOnChainInfo info;
    SectorNumber id;
  };

  struct MsgWait {
    MessageReceipt receipt;
    Tipset tipset;
  };

  struct BlockMessages {
    std::vector<UnsignedMessage> bls;
    std::vector<SignedMessage> secp;
    std::vector<CID> cids;
  };

  struct CidMessage {
    CID cid;
    UnsignedMessage message;
  };

  struct IpldObject {
    CID cid;
    Buffer raw;
  };

  struct MpoolUpdate {
    int64_t type;
    SignedMessage message;
  };

  struct VersionResult {
    std::string version;
    uint64_t api_version;
    uint64_t block_delay;
  };

  struct MiningBaseInfo {
    StoragePower miner_power;
    StoragePower network_power;
    std::vector<ChainSectorInfo> sectors;
    Address worker;
    SectorSize sector_size;
  };

  struct Api {
    API_METHOD(AuthNew, Buffer, const std::vector<std::string> &)

    API_METHOD(ChainGetBlock, BlockHeader, const CID &)
    API_METHOD(ChainGetBlockMessages, BlockMessages, const CID &)
    API_METHOD(ChainGetGenesis, Tipset)
    API_METHOD(ChainGetNode, IpldObject, const std::string &)
    API_METHOD(ChainGetParentMessages, std::vector<CidMessage>, const CID &)
    API_METHOD(ChainGetParentReceipts, std::vector<MessageReceipt>, const CID &)
    API_METHOD(ChainGetRandomness, Randomness, const TipsetKey &, int64_t)
    API_METHOD(ChainGetTipSet, Tipset, const TipsetKey &)
    API_METHOD(ChainGetTipSetByHeight, Tipset, ChainEpoch, const TipsetKey &)
    API_METHOD(ChainHead, Tipset)
    API_METHOD(ChainNotify, Chan<std::vector<HeadChange>>)
    API_METHOD(ChainReadObj, Buffer, CID)
    API_METHOD(ChainSetHead, void, const TipsetKey &)
    API_METHOD(ChainTipSetWeight, TipsetWeight, const TipsetKey &)

    API_METHOD(MarketEnsureAvailable, void, const Address &, TokenAmount)

    API_METHOD(MinerCreateBlock,
               BlockMsg,
               const Address &,
               const TipsetKey &,
               const Ticket &,
               const EPostProof &,
               const std::vector<SignedMessage> &,
               ChainEpoch,
               uint64_t)
    API_METHOD(MinerGetBaseInfo,
               MiningBaseInfo,
               const Address &,
               const TipsetKey &)

    API_METHOD(MpoolPending, std::vector<SignedMessage>, const TipsetKey &)
    API_METHOD(MpoolPushMessage, SignedMessage, const UnsignedMessage &)
    API_METHOD(MpoolSub, Chan<MpoolUpdate>)

    API_METHOD(NetAddrsListen, PeerInfo)

    API_METHOD(PaychVoucherAdd,
               TokenAmount,
               const Address &,
               const SignedVoucher &,
               const Buffer &,
               TokenAmount)

    API_METHOD(StateAccountKey, Address, const Address &, const TipsetKey &)
    API_METHOD(StateCall,
               InvocResult,
               const UnsignedMessage &,
               const TipsetKey &)
    API_METHOD(StateGetActor, Actor, const Address &, const TipsetKey &)
    API_METHOD(StateListMiners, std::vector<Address>, const TipsetKey &)
    API_METHOD(StateMarketBalance,
               StorageParticipantBalance,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMarketDeals, MarketDealMap, const TipsetKey &)
    API_METHOD(StateMarketStorageDeal, StorageDeal, DealId, const TipsetKey &)
    API_METHOD(StateMinerElectionPeriodStart,
               ChainEpoch,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerFaults, RleBitset, const Address &, const TipsetKey &)
    API_METHOD(StateMinerInfo, MinerInfo, const Address &, const TipsetKey &)
    API_METHOD(StateMinerPostState,
               PoStState,
               const Address &,
               const TipsetKey &)
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
    API_METHOD(StateNetworkName, std::string)
    API_METHOD(StateWaitMsg, Wait<MsgWait>, const CID &)

    API_METHOD(SyncSubmitBlock, void, const BlockMsg &)

    API_METHOD(Version, VersionResult)

    API_METHOD(WalletDefaultAddress, Address)
    API_METHOD(WalletHas, bool, const Address &)
    API_METHOD(WalletSign, Signature, const Address &, const Buffer &)
  };
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_API_HPP
