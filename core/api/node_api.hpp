/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "common/libp2p/peer/cbor_peer_id.hpp"
#include "const.hpp"
#include "drand/messages.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/block/block.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/payment_channel/types.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::api {
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using drand::BeaconEntry;
  using libp2p::peer::PeerId;
  using markets::storage::DataRef;
  using markets::storage::SignedStorageAsk;
  using markets::storage::StorageDeal;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::EpochDuration;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StoragePower;
  using primitives::TipsetWeight;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::block::BlockTemplate;
  using primitives::block::BlockWithCids;
  using primitives::sector::SectorInfo;
  using primitives::tipset::HeadChange;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetKey;
  using storage::mpool::MpoolUpdate;
  using vm::actor::Actor;
  using vm::actor::builtin::types::storage_power::Claim;
  using vm::actor::builtin::v0::miner::DeadlineInfo;
  using vm::actor::builtin::v0::miner::Deadlines;
  using vm::actor::builtin::v0::miner::MinerInfo;
  using vm::actor::builtin::v0::miner::SectorOnChainInfo;
  using vm::actor::builtin::v0::miner::SectorPreCommitInfo;
  using vm::actor::builtin::v0::miner::SectorPreCommitOnChainInfo;
  using vm::actor::builtin::v0::payment_channel::LaneId;
  using vm::actor::builtin::v0::payment_channel::SignedVoucher;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using vm::version::NetworkVersion;
  using SignatureType = crypto::signature::Type;

  struct None {};

  struct InvocResult {
    UnsignedMessage message;
    MessageReceipt receipt;
    std::string error;
  };

  using MarketDealMap = std::map<std::string, StorageDeal>;

  struct FileRef {
    std::string path;
    bool is_car;
  };

  struct RetrievalOrder {
    CID root;
    uint64_t size;
    TokenAmount total;
    uint64_t interval;
    uint64_t interval_inc;
    Address client;
    Address miner;
    PeerId peer{codec::cbor::kDefaultT<PeerId>()};
  };

  struct StartDealParams {
    DataRef data;
    Address wallet;
    Address miner;
    TokenAmount epoch_price;
    EpochDuration min_blocks_duration;
    ChainEpoch deal_start_epoch;
  };

  struct MarketBalance {
    TokenAmount escrow, locked;
  };

  struct QueryOffer {
    std::string error;
    CID root;
    uint64_t size;
    TokenAmount min_price;
    uint64_t payment_interval;
    uint64_t payment_interval_increase;
    Address miner;
    PeerId peer;
  };

  struct Import {
    int64_t status;
    CID key;
    std::string path;
    uint64_t size;
  };

  struct AddChannelInfo {
    Address channel;      // payment channel actor address
    CID channel_message;  // message cid
  };

  struct KeyInfo {
    SignatureType type;
    common::Blob<32> private_key;
  };

  struct Partition {
    RleBitset all, faulty, recovering, live, active;
  };

  struct SectorLocation {
    uint64_t deadline;
    uint64_t partition;
  };

  struct MinerPower {
    Claim miner;
    Claim total;
  };

  struct MsgWait {
    CID message;
    MessageReceipt receipt;
    TipsetKey tipset;
    ChainEpoch height;
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

  struct MiningBaseInfo {
    StoragePower miner_power;
    StoragePower network_power;
    std::vector<SectorInfo> sectors;
    Address worker;
    SectorSize sector_size;
    BeaconEntry prev_beacon;
    std::vector<BeaconEntry> beacons;
    bool has_min_power;

    auto &beacon() const {
      return beacons.empty() ? prev_beacon : beacons.back();
    }
  };

  struct ActorState {
    BigInt balance;
    IpldObject state;
  };

  struct MessageSendSpec {
    static TokenAmount maxFee(const boost::optional<MessageSendSpec> &spec) {
      if (spec) {
        return spec->max_fee;
      }
      return kFilecoinPrecision / 10;
    };
    TokenAmount max_fee;
  };

  inline const boost::optional<MessageSendSpec> kPushNoSpec;

  constexpr uint64_t kNoConfidence{};

  struct FullNodeApi : public CommonApi {
    API_METHOD(BeaconGetEntry, Wait<BeaconEntry>, ChainEpoch)

    API_METHOD(ChainGetBlock, BlockHeader, const CID &)
    API_METHOD(ChainGetBlockMessages, BlockMessages, const CID &)
    API_METHOD(ChainGetGenesis, TipsetCPtr)
    API_METHOD(ChainGetNode, IpldObject, const std::string &)
    API_METHOD(ChainGetMessage, UnsignedMessage, const CID &)
    API_METHOD(ChainGetParentMessages, std::vector<CidMessage>, const CID &)
    API_METHOD(ChainGetParentReceipts, std::vector<MessageReceipt>, const CID &)
    API_METHOD(ChainGetRandomnessFromBeacon,
               Randomness,
               const TipsetKey &,
               DomainSeparationTag,
               ChainEpoch,
               const Buffer &)
    API_METHOD(ChainGetRandomnessFromTickets,
               Randomness,
               const TipsetKey &,
               DomainSeparationTag,
               ChainEpoch,
               const Buffer &)
    API_METHOD(ChainGetTipSet, TipsetCPtr, const TipsetKey &)
    API_METHOD(ChainGetTipSetByHeight,
               TipsetCPtr,
               ChainEpoch,
               const TipsetKey &)
    API_METHOD(ChainHead, TipsetCPtr)
    API_METHOD(ChainNotify, Chan<std::vector<HeadChange>>)
    API_METHOD(ChainReadObj, Buffer, CID)
    API_METHOD(ChainSetHead, void, const TipsetKey &)
    API_METHOD(ChainTipSetWeight, TipsetWeight, const TipsetKey &)

    API_METHOD(ClientFindData, Wait<std::vector<QueryOffer>>, const CID &)
    API_METHOD(ClientHasLocal, bool, const CID &)
    API_METHOD(ClientImport, CID, const FileRef &)
    API_METHOD(ClientListImports, std::vector<Import>)
    API_METHOD(ClientQueryAsk,
               Wait<SignedStorageAsk>,
               const std::string &,
               const Address &)
    API_METHOD(ClientRetrieve,
               Wait<None>,
               const RetrievalOrder &,
               const FileRef &)
    API_METHOD(ClientStartDeal, Wait<CID>, const StartDealParams &)

    API_METHOD(GasEstimateMessageGas,
               UnsignedMessage,
               const UnsignedMessage &,
               const boost::optional<MessageSendSpec> &,
               const TipsetKey &)

    /**
     * Ensures that a storage market participant has a certain amount of
     * available funds. If additional funds are needed, they will be sent from
     * the 'wallet' address callback is immediately called if sufficient funds
     * are available
     * @param wallet to send from
     * @param address to ensure
     * @param amount to ensure
     * @return CID of transfer message if message was sent
     */
    API_METHOD(MarketReserveFunds,
               boost::optional<CID>,
               const Address &,
               const Address &,
               const TokenAmount &)

    API_METHOD(MinerCreateBlock, BlockWithCids, const BlockTemplate &)
    API_METHOD(MinerGetBaseInfo,
               Wait<boost::optional<MiningBaseInfo>>,
               const Address &,
               ChainEpoch,
               const TipsetKey &)

    API_METHOD(MpoolPending, std::vector<SignedMessage>, const TipsetKey &)
    API_METHOD(MpoolPushMessage,
               SignedMessage,
               const UnsignedMessage &,
               const boost::optional<MessageSendSpec> &)
    API_METHOD(MpoolSelect,
               std::vector<SignedMessage>,
               const TipsetKey &,
               double)
    API_METHOD(MpoolSub, Chan<MpoolUpdate>)

    /** Payment channel manager */

    /**
     * Allocate new payment channel lane
     * @param payment channel actor address
     * @return new lane id
     */
    API_METHOD(PaychAllocateLane, LaneId, const Address &)

    /**
     * Get or create payment channel and waits for message is committed
     * Search for payment channel in local storage.
     * If found, adds ensure_funds to payment channel actor.
     * If not found, creates payment channel actor with ensure_funds
     * @param from address
     * @param to address
     * @param ensure_funds - amount allocated for payment channel
     * @return add payment channel info with actor address and message cid
     */
    API_METHOD(PaychGet,
               AddChannelInfo,
               const Address &,
               const Address &,
               const TokenAmount &)

    /**
     * Add voucher to local storage
     * @param payment channel address
     * @param signed voucher
     * @param signature one more time - not used
     * @param delta - not used
     * @return delta
     */
    API_METHOD(PaychVoucherAdd,
               TokenAmount,
               const Address &,
               const SignedVoucher &,
               const Buffer &,
               const TokenAmount &)

    /**
     * Validate voucher
     * @param payment channel actor address
     * @param voucher to validate
     */
    API_METHOD(PaychVoucherCheckValid,
               void,
               const Address &,
               const SignedVoucher &)

    /**
     * Creates voucher for payment channel lane
     * @param payment channel actor address
     * @param token amound to redeem
     * @param lane id
     * @return signed voucher
     */
    API_METHOD(PaychVoucherCreate,
               SignedVoucher,
               const Address &,
               const TokenAmount &,
               const LaneId &)

    API_METHOD(StateAccountKey, Address, const Address &, const TipsetKey &)
    API_METHOD(StateCall,
               InvocResult,
               const UnsignedMessage &,
               const TipsetKey &)
    API_METHOD(StateListMessages,
               std::vector<CID>,
               const UnsignedMessage &,
               const TipsetKey &,
               ChainEpoch)
    API_METHOD(StateGetActor, Actor, const Address &, const TipsetKey &)
    API_METHOD(StateReadState, ActorState, const Actor &, const TipsetKey &)
    API_METHOD(StateGetReceipt, MessageReceipt, const CID &, const TipsetKey &)
    API_METHOD(StateListMiners, std::vector<Address>, const TipsetKey &)
    API_METHOD(StateListActors, std::vector<Address>, const TipsetKey &)
    API_METHOD(StateMarketBalance,
               MarketBalance,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMarketDeals, MarketDealMap, const TipsetKey &)
    API_METHOD(StateLookupID, Address, const Address &, const TipsetKey &)
    API_METHOD(StateMarketStorageDeal, StorageDeal, DealId, const TipsetKey &)
    API_METHOD(StateMinerDeadlines,
               Deadlines,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerFaults, RleBitset, const Address &, const TipsetKey &)
    API_METHOD(StateMinerInfo, MinerInfo, const Address &, const TipsetKey &)
    API_METHOD(StateMinerPartitions,
               std::vector<Partition>,
               const Address &,
               uint64_t,
               const TipsetKey &)
    API_METHOD(StateMinerPower, MinerPower, const Address &, const TipsetKey &)
    API_METHOD(StateMinerProvingDeadline,
               DeadlineInfo,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerSectors,
               std::vector<SectorOnChainInfo>,
               const Address &,
               const boost::optional<RleBitset> &,
               const TipsetKey &)
    API_METHOD(StateNetworkName, std::string)
    API_METHOD(StateNetworkVersion, NetworkVersion, const TipsetKey &)
    API_METHOD(StateMinerPreCommitDepositForPower,
               TokenAmount,
               const Address &,
               const SectorPreCommitInfo &,
               const TipsetKey &)
    API_METHOD(StateMinerInitialPledgeCollateral,
               TokenAmount,
               const Address &,
               const SectorPreCommitInfo &,
               const TipsetKey &);
    API_METHOD(StateSectorPreCommitInfo,
               SectorPreCommitOnChainInfo,
               const Address &,
               SectorNumber,
               const TipsetKey &);
    API_METHOD(StateSectorGetInfo,
               boost::optional<SectorOnChainInfo>,
               const Address &,
               SectorNumber,
               const TipsetKey &);
    API_METHOD(StateSectorPartition,
               SectorLocation,
               const Address &,
               SectorNumber,
               const TipsetKey &);
    API_METHOD(StateSearchMsg, boost::optional<MsgWait>, const CID &)
    API_METHOD(StateWaitMsg, Wait<MsgWait>, const CID &, uint64_t)

    API_METHOD(SyncSubmitBlock, void, const BlockWithCids &)

    /** Wallet */
    API_METHOD(WalletBalance, TokenAmount, const Address &)
    API_METHOD(WalletDefaultAddress, Address)
    API_METHOD(WalletHas, bool, const Address &)
    API_METHOD(WalletImport, Address, const KeyInfo &)
    API_METHOD(WalletSign, Signature, const Address &, const Buffer &)
    /** Verify signature by address (may be id or key address) */
    API_METHOD(
        WalletVerify, bool, const Address &, const Buffer &, const Signature &)
  };
}  // namespace fc::api
