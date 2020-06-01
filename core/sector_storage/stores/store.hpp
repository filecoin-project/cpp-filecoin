/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORE_HPP
#define CPP_FILECOIN_STORE_HPP

#include <functional>
#include "primitives/sector_file/sector_file.hpp"
#include "common/outcome.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {

  using primitives::FsStat;
  using primitives::StorageID;
  using primitives::sector_file::SectorFileType;
  using primitives::sector_file::SectorPaths;

  struct AcquireSectorResponse {
    SectorPaths paths;
    SectorPaths stores;
    std::function<void()> release;
  };

  class Store {
   public:
    virtual ~Store() = default;

    virtual outcome::result<AcquireSectorResponse> acquireSector(
        SectorId sector,
        RegisteredProof seal_proof_type,
        SectorFileType existing,
        SectorFileType allocate,
        bool can_seal) = 0;

    virtual outcome::result<void> remove(SectorId sector,
                                         SectorFileType types,
                                         bool force) = 0;

    virtual outcome::result<void> moveStorage(SectorId sector,
                                              RegisteredProof seal_proof_type,
                                              SectorFileType types) = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID id) = 0;
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_STORE_HPP
