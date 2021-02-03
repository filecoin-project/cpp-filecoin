/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "markets/retrieval/types.hpp"
#include "miner/storage_fsm/types.hpp"

namespace fc::api {
  using markets::retrieval::RetrievalAsk;
  using mining::types::DealInfo;
  using mining::types::DealSchedule;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using primitives::piece::PaddedPieceSize;

  struct PieceLocation {
    SectorNumber sector_number;
    PaddedPieceSize offset;
    PaddedPieceSize length;
  };

  struct StorageMinerApi : public CommonApi {
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
  };
}  // namespace fc::api
