/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "common/libp2p/peer/cbor_peer_id.hpp"
#include "const.hpp"
#include "data_transfer/types.hpp"
#include "drand/messages.hpp"
#include "markets/retrieval/types.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/client/import_manager/import_manager.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/block/block.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/actor/builtin/types/miner/deadline_info.hpp"
#include "vm/actor/builtin/types/miner/deadlines.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"
#include "vm/actor/builtin/types/storage_power/claim.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::api {
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using data_transfer::TransferId;
  using drand::BeaconEntry;
  using libp2p::multi::Multiaddress;
  using libp2p::peer::PeerId;
  using markets::retrieval::RetrievalPeer;
  using markets::storage::DataRef;
  using markets::storage::SignedStorageAsk;
  using markets::storage::StorageDeal;
  using markets::storage::StorageDealStatus;
  using markets::storage::client::import_manager::Import;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::EpochDuration;
  using primitives::GasAmount;
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
  using primitives::piece::PaddedPieceSize;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SectorInfo;
  using primitives::tipset::HeadChange;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetKey;
  using storage::mpool::MpoolUpdate;
  using vm::actor::Actor;
  using vm::actor::builtin::types::miner::DeadlineInfo;
  using vm::actor::builtin::types::miner::Deadlines;
  using vm::actor::builtin::types::miner::SectorOnChainInfo;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo;
  using vm::actor::builtin::types::payment_channel::LaneId;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;
  using vm::actor::builtin::types::storage_power::Claim;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using vm::version::NetworkVersion;
  using SignatureType = crypto::signature::Type;

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

  /** Unique identifier for a channel */
  struct ChannelId {
    PeerId initiator{codec::cbor::kDefaultT<PeerId>()};
    PeerId responder{codec::cbor::kDefaultT<PeerId>()};
    TransferId id;
  };

  struct DatatransferChannel {
    TransferId transfer_id;
    uint64_t status;
    CID base_cid;
    bool is_initiator;
    bool is_sender;
    std::string voucher;
    std::string message;
    PeerId other_peer{codec::cbor::kDefaultT<PeerId>()};
    uint64_t transferred;
  };

  struct StorageMarketDealInfo {
    CID proposal_cid;
    StorageDealStatus state;
    std::string message;
    Address provider;
    DataRef data_ref;
    CID piece_cid;
    uint64_t size;
    TokenAmount price_per_epoch;
    EpochDuration duration;
    DealId deal_id;
    uint64_t creation_time;
    bool verified;
    ChannelId transfer_channel_id;
    DatatransferChannel data_transfer;
  };

  /**
   * Client import response
   */
  struct ImportRes {
    /** root CID of imported data */
    CID root;

    /**
     * Storage id of multistorage in Lotus
     * Not supported in Fuhon, returns 0
     */
    uint64_t import_id;
  };

  struct RetrievalOrder {
    CID root;
    boost::optional<CID> piece;
    uint64_t size;
    /** StoreId of multistore (not implemented in Fuhon) */
    boost::optional<uint64_t> local_store;
    TokenAmount total;
    TokenAmount unseal_price;
    uint64_t payment_interval;
    uint64_t payment_interval_increase;
    Address client;
    Address miner;
    boost::optional<RetrievalPeer> peer;
  };

  struct StartDealParams {
    DataRef data;
    Address wallet;
    Address miner;
    TokenAmount epoch_price;
    EpochDuration min_blocks_duration;
    TokenAmount provider_collateral;
    ChainEpoch deal_start_epoch;
    bool fast_retrieval;
    bool verified_deal;
  };

  struct MarketBalance {
    TokenAmount escrow, locked;
  };

  struct QueryOffer {
    std::string error;
    CID root;
    boost::optional<CID> piece;
    uint64_t size;
    TokenAmount min_price;
    TokenAmount unseal_price;
    uint64_t payment_interval;
    uint64_t payment_interval_increase;
    Address miner;
    RetrievalPeer peer;
  };

  struct AddChannelInfo {
    Address channel;      // payment channel actor address
    CID channel_message;  // message cid
  };

  struct KeyInfo {
    SignatureType type = SignatureType::kUndefined;
    common::Blob<32> private_key;
  };

  struct Partition {
    RleBitset all;
    RleBitset faulty;
    RleBitset recovering;
    RleBitset live;
    RleBitset active;
  };

  struct Deadline {
    RleBitset partitions_posted;
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
    ChainEpoch height{};
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
    Bytes raw;
  };

  struct MiningBaseInfo {
    StoragePower miner_power;
    StoragePower network_power;
    std::vector<SectorInfo> sectors;
    Address worker;
    SectorSize sector_size{};
    BeaconEntry prev_beacon;
    std::vector<BeaconEntry> beacons;
    bool has_min_power = false;

    auto &beacon() const {
      return beacons.empty() ? prev_beacon : beacons.back();
    }
  };

  struct ActorState {
    BigInt balance;
    IpldObject state;
  };

  struct MessageSendSpec {
    TokenAmount max_fee;
  };

  struct MinerInfo {
    Address owner;
    Address worker;
    std::vector<Address> control;
    Bytes peer_id;
    std::vector<Multiaddress> multiaddrs;
    RegisteredPoStProof window_post_proof_type{RegisteredPoStProof::kUndefined};
    SectorSize sector_size{};
    uint64_t window_post_partition_sectors{};
  };

  struct DealCollateralBounds {
    TokenAmount min;
    TokenAmount max;
  };

  inline const boost::optional<MessageSendSpec> kPushNoSpec;

  constexpr uint64_t kNoConfidence{};

  constexpr ChainEpoch kLookbackNoLimit{-1};

  /**
   * FullNode API is a low-level interface to the Filecoin network full node.
   * Provides the latest node API v2.0.0
   */
  struct FullNodeApi : public CommonApi {
    /**
     * @note long operation
     */
    API_METHOD(BeaconGetEntry, jwt::kReadPermission, BeaconEntry, ChainEpoch)

    API_METHOD(ChainGetBlock, jwt::kReadPermission, BlockHeader, const CID &)
    API_METHOD(ChainGetBlockMessages,
               jwt::kReadPermission,
               BlockMessages,
               const CID &)
    API_METHOD(ChainGetGenesis, jwt::kReadPermission, TipsetCPtr)
    API_METHOD(ChainGetNode,
               jwt::kReadPermission,
               IpldObject,
               const std::string &)
    API_METHOD(ChainGetMessage,
               jwt::kReadPermission,
               UnsignedMessage,
               const CID &)
    API_METHOD(ChainGetParentMessages,
               jwt::kReadPermission,
               std::vector<CidMessage>,
               const CID &)
    API_METHOD(ChainGetParentReceipts,
               jwt::kReadPermission,
               std::vector<MessageReceipt>,
               const CID &)
    API_METHOD(ChainGetRandomnessFromBeacon,
               jwt::kReadPermission,
               Randomness,
               const TipsetKey &,
               DomainSeparationTag,
               ChainEpoch,
               const Bytes &)
    API_METHOD(ChainGetRandomnessFromTickets,
               jwt::kReadPermission,
               Randomness,
               const TipsetKey &,
               DomainSeparationTag,
               ChainEpoch,
               const Bytes &)
    API_METHOD(ChainGetTipSet,
               jwt::kReadPermission,
               TipsetCPtr,
               const TipsetKey &)
    API_METHOD(ChainGetTipSetByHeight,
               jwt::kReadPermission,
               TipsetCPtr,
               ChainEpoch,
               const TipsetKey &)
    API_METHOD(ChainHead, jwt::kReadPermission, TipsetCPtr)
    API_METHOD(ChainNotify, jwt::kReadPermission, Chan<std::vector<HeadChange>>)
    API_METHOD(ChainReadObj, jwt::kReadPermission, Bytes, CID)
    API_METHOD(ChainSetHead, jwt::kAdminPermission, void, const TipsetKey &)
    API_METHOD(ChainTipSetWeight,
               jwt::kReadPermission,
               TipsetWeight,
               const TipsetKey &)

    /**
     * Identifies peers that have a certain file, and returns QueryOffers for
     * each peer.
     * @param data root cid
     * @param optional piece cid
     * @note long operation
     */
    API_METHOD(ClientFindData,
               jwt::kReadPermission,
               std::vector<QueryOffer>,
               const CID &,
               const boost::optional<CID> &)
    API_METHOD(ClientHasLocal, jwt::kWritePermission, bool, const CID &)

    /**
     * Imports file under the specified path into Storage Market Client
     * filestore.
     * @param FielRef - path to the file and flag if the file is a CAR. CAR file
     * must be a one single root CAR.
     * @return CID - root CID to the data
     */
    API_METHOD(ClientImport, jwt::kWritePermission, ImportRes, const FileRef &)

    /**
     * Returns information about the deals made by the local client
     */
    API_METHOD(ClientListDeals,
               jwt::kWritePermission,
               std::vector<StorageMarketDealInfo>)

    /**
     * Lists imported files and their root CIDs
     */
    API_METHOD(ClientListImports, jwt::kWritePermission, std::vector<Import>)
    /**
     * @note long operation
     */
    API_METHOD(ClientMinerQueryOffer,
               jwt::kReadPermission,
               QueryOffer,
               const Address &,
               const CID &,
               const boost::optional<CID> &)
    /**
     * @note long operation
     */
    API_METHOD(ClientQueryAsk,
               jwt::kReadPermission,
               SignedStorageAsk,
               const std::string &,
               const Address &)

    /**
     * Initiates the retrieval of a file, as specified in the order
     * @note long operation
     */
    API_METHOD(ClientRetrieve,
               jwt::kAdminPermission,
               void,
               const RetrievalOrder &,
               const FileRef &)

    /**
     * Proposes a storage deal with a miner
     */
    API_METHOD(ClientStartDeal,
               jwt::kAdminPermission,
               CID,
               const StartDealParams &)

    API_METHOD(GasEstimateFeeCap,
               jwt::kReadPermission,
               TokenAmount,
               const UnsignedMessage &,
               int64_t,
               const TipsetKey &)
    API_METHOD(GasEstimateGasPremium,
               jwt::kReadPermission,
               TokenAmount,
               uint64_t,
               const Address &,
               GasAmount,
               const TipsetKey &)
    API_METHOD(GasEstimateMessageGas,
               jwt::kReadPermission,
               UnsignedMessage,
               const UnsignedMessage &,
               const boost::optional<MessageSendSpec> &,
               const TipsetKey &)

    /**
     * Ensures that a storage market participant has a certain amount of
     * available funds. If additional funds are needed, they will be sent from
     * the 'wallet' address.
     * @param wallet to send from
     * @param address to ensure
     * @param amount to ensure
     * @return CID of transfer message if message was sent or boost::none if
     * required funds were already available
     */
    API_METHOD(MarketReserveFunds,
               jwt::kSignPermission,
               boost::optional<CID>,
               const Address &,
               const Address &,
               const TokenAmount &)

    API_METHOD(MinerCreateBlock,
               jwt::kWritePermission,
               BlockWithCids,
               const BlockTemplate &)
    /**
     * @note long operation
     */
    API_METHOD(MinerGetBaseInfo,
               jwt::kReadPermission,
               boost::optional<MiningBaseInfo>,
               const Address &,
               ChainEpoch,
               const TipsetKey &)

    API_METHOD(MpoolPending,
               jwt::kReadPermission,
               std::vector<SignedMessage>,
               const TipsetKey &)
    API_METHOD(MpoolPushMessage,
               jwt::kSignPermission,
               SignedMessage,
               const UnsignedMessage &,
               const boost::optional<MessageSendSpec> &)
    API_METHOD(MpoolSelect,
               jwt::kReadPermission,
               std::vector<SignedMessage>,
               const TipsetKey &,
               double)
    API_METHOD(MpoolSub, jwt::kReadPermission, Chan<MpoolUpdate>)

    /** Payment channel manager */

    /**
     * Allocate new payment channel lane
     * @param payment channel actor address
     * @return new lane id
     */
    API_METHOD(PaychAllocateLane, jwt::kSignPermission, LaneId, const Address &)

    /**
     * Get or create payment channel and waits for message is committed
     * Search for payment channel in local storage.
     * If found, adds ensure_funds to payment channel actor.
     * If not found, creates payment channel actor with ensure_funds
     * @param from address
     * @param to address
     * @param ensure_funds - amount allocated for payment channel
     * @return add payment channel info with actor address and message cid
     * @note long operation
     */
    API_METHOD(PaychGet,
               jwt::kSignPermission,
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
               jwt::kWritePermission,
               TokenAmount,
               const Address &,
               const SignedVoucher &,
               const Bytes &,
               const TokenAmount &)

    /**
     * Validate voucher
     * @param payment channel actor address
     * @param voucher to validate
     */
    API_METHOD(PaychVoucherCheckValid,
               jwt::kReadPermission,
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
               jwt::kSignPermission,
               SignedVoucher,
               const Address &,
               const TokenAmount &,
               const LaneId &)

    API_METHOD(StateAccountKey,
               jwt::kReadPermission,
               Address,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateCall,
               jwt::kReadPermission,
               InvocResult,
               const UnsignedMessage &,
               const TipsetKey &)
    API_METHOD(StateDealProviderCollateralBounds,
               jwt::kReadPermission,
               DealCollateralBounds,
               PaddedPieceSize,
               bool,
               const TipsetKey &)
    API_METHOD(StateListMessages,
               jwt::kReadPermission,
               std::vector<CID>,
               const UnsignedMessage &,
               const TipsetKey &,
               ChainEpoch)
    API_METHOD(StateGetActor,
               jwt::kReadPermission,
               Actor,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateReadState,
               jwt::kReadPermission,
               ActorState,
               const Actor &,
               const TipsetKey &)
    API_METHOD(StateListMiners,
               jwt::kReadPermission,
               std::vector<Address>,
               const TipsetKey &)
    API_METHOD(StateListActors,
               jwt::kReadPermission,
               std::vector<Address>,
               const TipsetKey &)
    API_METHOD(StateMarketBalance,
               jwt::kReadPermission,
               MarketBalance,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMarketDeals,
               jwt::kReadPermission,
               MarketDealMap,
               const TipsetKey &)
    API_METHOD(StateLookupID,
               jwt::kReadPermission,
               Address,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMarketStorageDeal,
               jwt::kReadPermission,
               StorageDeal,
               DealId,
               const TipsetKey &)

    API_METHOD(StateMinerActiveSectors,
               jwt::kReadPermission,
               std::vector<SectorOnChainInfo>,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerAvailableBalance,
               jwt::kReadPermission,
               TokenAmount,
               const Address &,
               const TipsetKey &)
    /** Returns PoSt submissions since the proving period started. */
    API_METHOD(StateMinerDeadlines,
               jwt::kReadPermission,
               std::vector<Deadline>,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerFaults,
               jwt::kReadPermission,
               RleBitset,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerInfo,
               jwt::kReadPermission,
               MinerInfo,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerPartitions,
               jwt::kReadPermission,
               std::vector<Partition>,
               const Address &,
               uint64_t,
               const TipsetKey &)
    API_METHOD(StateMinerPower,
               jwt::kReadPermission,
               MinerPower,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerProvingDeadline,
               jwt::kReadPermission,
               DeadlineInfo,
               const Address &,
               const TipsetKey &)
    API_METHOD(StateMinerSectorAllocated,
               jwt::kReadPermission,
               bool,
               const Address &,
               SectorNumber,
               const TipsetKey &)
    API_METHOD(StateMinerSectors,
               jwt::kReadPermission,
               std::vector<SectorOnChainInfo>,
               const Address &,
               const boost::optional<RleBitset> &,
               const TipsetKey &)
    API_METHOD(StateNetworkName, jwt::kReadPermission, std::string)
    API_METHOD(StateNetworkVersion,
               jwt::kReadPermission,
               NetworkVersion,
               const TipsetKey &)
    API_METHOD(StateMinerPreCommitDepositForPower,
               jwt::kReadPermission,
               TokenAmount,
               const Address &,
               const SectorPreCommitInfo &,
               const TipsetKey &)
    API_METHOD(StateMinerInitialPledgeCollateral,
               jwt::kReadPermission,
               TokenAmount,
               const Address &,
               const SectorPreCommitInfo &,
               const TipsetKey &);

    /**
     * Gets the current seal proof type for the given miner
     * @param Address - miner address
     * @param tipset
     * @return preferred registered seal proof type
     */
    API_METHOD(GetProofType,
               jwt::kReadPermission,
               RegisteredSealProof,
               const Address &,
               const TipsetKey &);

    API_METHOD(StateSectorPreCommitInfo,
               jwt::kReadPermission,
               SectorPreCommitOnChainInfo,
               const Address &,
               SectorNumber,
               const TipsetKey &);
    API_METHOD(StateSectorGetInfo,
               jwt::kReadPermission,
               boost::optional<SectorOnChainInfo>,
               const Address &,
               SectorNumber,
               const TipsetKey &);
    API_METHOD(StateSectorPartition,
               jwt::kReadPermission,
               SectorLocation,
               const Address &,
               SectorNumber,
               const TipsetKey &);

    /**
     * Verified registry actor state method
     * @return the data cap for the given address
     */
    API_METHOD(StateVerifiedClientStatus,
               jwt::kReadPermission,
               boost::optional<StoragePower>,
               const Address &,
               const TipsetKey &)

    /**
     * @note long operation
     */
    API_METHOD(StateSearchMsg,
               jwt::kReadPermission,
               boost::optional<MsgWait>,
               const TipsetKey &,
               const CID &,
               ChainEpoch,
               bool)
    /**
     * @note long operation
     */
    API_METHOD(StateWaitMsg,
               jwt::kReadPermission,
               MsgWait,
               const CID &,
               uint64_t,
               ChainEpoch,
               bool)

    API_METHOD(SyncSubmitBlock,
               jwt::kWritePermission,
               void,
               const BlockWithCids &)

    /** Wallet */
    API_METHOD(WalletBalance,
               jwt::kReadPermission,
               TokenAmount,
               const Address &)
    API_METHOD(WalletDefaultAddress, jwt::kWritePermission, Address)
    API_METHOD(WalletHas, jwt::kWritePermission, bool, const Address &)
    API_METHOD(WalletImport, jwt::kAdminPermission, Address, const KeyInfo &)
    API_METHOD(WalletNew, jwt::kWritePermission, Address, const std::string &)
    API_METHOD(WalletSetDefault, jwt::kWritePermission, void, const Address &)
    API_METHOD(WalletSign,
               jwt::kSignPermission,
               Signature,
               const Address &,
               const Bytes &)
    /** Verify signature by address (may be id or key address) */
    API_METHOD(WalletVerify,
               jwt::kReadPermission,
               bool,
               const Address &,
               const Bytes &,
               const Signature &)
  };

  template <typename A, typename F>
  void visit(const FullNodeApi &, A &&a, const F &f) {
    visitCommon(a, f);
    f(a.BeaconGetEntry);
    f(a.ChainGetBlock);
    f(a.ChainGetBlockMessages);
    f(a.ChainGetGenesis);
    f(a.ChainGetMessage);
    f(a.ChainGetNode);
    f(a.ChainGetParentMessages);
    f(a.ChainGetParentReceipts);
    f(a.ChainGetRandomnessFromBeacon);
    f(a.ChainGetRandomnessFromTickets);
    f(a.ChainGetTipSet);
    f(a.ChainGetTipSetByHeight);
    f(a.ChainHead);
    f(a.ChainNotify);
    f(a.ChainReadObj);
    f(a.ChainSetHead);
    f(a.ChainTipSetWeight);
    f(a.ClientFindData);
    f(a.ClientHasLocal);
    f(a.ClientImport);
    f(a.ClientListDeals);
    f(a.ClientListImports);
    f(a.ClientMinerQueryOffer);
    f(a.ClientQueryAsk);
    f(a.ClientRetrieve);
    f(a.ClientStartDeal);
    f(a.GasEstimateFeeCap);
    f(a.GasEstimateGasPremium);
    f(a.GasEstimateMessageGas);
    f(a.MarketReserveFunds);
    f(a.MinerCreateBlock);
    f(a.MinerGetBaseInfo);
    f(a.MpoolPending);
    f(a.MpoolPushMessage);
    f(a.MpoolSelect);
    f(a.MpoolSub);
    f(a.PaychAllocateLane);
    f(a.PaychGet);
    f(a.PaychVoucherAdd);
    f(a.PaychVoucherCheckValid);
    f(a.PaychVoucherCreate);
    f(a.StateAccountKey);
    f(a.StateCall);
    f(a.StateDealProviderCollateralBounds);
    f(a.StateGetActor);
    f(a.StateListActors);
    f(a.StateListMessages);
    f(a.StateListMiners);
    f(a.StateLookupID);
    f(a.StateMarketBalance);
    f(a.StateMarketDeals);
    f(a.StateMarketStorageDeal);
    f(a.StateMinerActiveSectors);
    f(a.StateMinerAvailableBalance);
    f(a.StateMinerDeadlines);
    f(a.StateMinerFaults);
    f(a.StateMinerInfo);
    f(a.StateMinerInitialPledgeCollateral);
    f(a.StateMinerPartitions);
    f(a.StateMinerPower);
    f(a.StateMinerPreCommitDepositForPower);
    f(a.StateMinerProvingDeadline);
    f(a.StateMinerSectorAllocated);
    f(a.StateMinerSectors);
    f(a.StateNetworkName);
    f(a.StateNetworkVersion);
    f(a.StateReadState);
    f(a.StateSearchMsg);
    f(a.StateSectorGetInfo);
    f(a.StateSectorPartition);
    f(a.StateVerifiedClientStatus);
    f(a.GetProofType);
    f(a.StateSectorPreCommitInfo);
    f(a.StateWaitMsg);
    f(a.SyncSubmitBlock);
    f(a.WalletBalance);
    f(a.WalletDefaultAddress);
    f(a.WalletHas);
    f(a.WalletImport);
    f(a.WalletNew);
    f(a.WalletSetDefault);
    f(a.WalletSign);
    f(a.WalletVerify);
  }
}  // namespace fc::api
