/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "markets/retrieval/types.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "miner/storage_fsm/types.hpp"

namespace fc::api {
  using markets::retrieval::RetrievalAsk;
  using markets::storage::SignedStorageAsk;
  using mining::types::DealInfo;
  using mining::types::DealSchedule;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;
  using StorageInfo_ = sector_storage::stores::StorageInfo;
  using primitives::StorageID;
  using primitives::sector_file::SectorFileType;
  using sector_storage::stores::FsStat;
  using sector_storage::stores::HealthReport;

  struct PieceLocation {
    SectorNumber sector_number;
    PaddedPieceSize offset;
    PaddedPieceSize length;
  };

  inline bool operator==(const PieceLocation &lhs, const PieceLocation &rhs) {
    return lhs.sector_number == rhs.sector_number && lhs.offset == rhs.offset
           && lhs.length == rhs.length;
  }

  struct StorageMinerApi : public CommonApi {
    API_METHOD(ActorAddress, Address)

    API_METHOD(ActorSectorSize, SectorSize, Address)

    API_METHOD(PledgeSector, void)

    API_METHOD(DealsImportData, void, const CID &, const std::string &)

    API_METHOD(MarketGetAsk, SignedStorageAsk)
    API_METHOD(MarketGetRetrievalAsk, RetrievalAsk)
    API_METHOD(MarketSetAsk,
               void,
               const TokenAmount &,
               const TokenAmount &,
               ChainEpoch,
               PaddedPieceSize,
               PaddedPieceSize)

    API_METHOD(MarketSetRetrievalAsk, void, const RetrievalAsk &)

    // TODO(ortyomka): [FIL-347] remove it
    API_METHOD(SealProof, RegisteredSealProof)

    API_METHOD(StorageAttach, void, const StorageInfo_ &, const FsStat &)
    API_METHOD(StorageInfo, StorageInfo_, const StorageID &)
    API_METHOD(StorageReportHealth,
               void,
               const StorageID &,
               const HealthReport &)
    API_METHOD(StorageDeclareSector,
               void,
               const StorageID &,
               const SectorId &,
               const SectorFileType &,
               bool)
    API_METHOD(StorageDropSector,
               void,
               const StorageID &,
               const SectorId &,
               const SectorFileType &)
    API_METHOD(StorageFindSector,
               std::vector<StorageInfo_>,
               const SectorId &,
               const SectorFileType &,
               boost::optional<RegisteredSealProof>)
    API_METHOD(StorageBestAlloc,
               std::vector<StorageInfo_>,
               const SectorFileType &,
               RegisteredSealProof,
               bool)

    API_METHOD(WorkerConnect, void, const std::string &);
  };
}  // namespace fc::api
