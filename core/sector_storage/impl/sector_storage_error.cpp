/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/sector_storage_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, SectorStorageError, e) {
    using fc::sector_storage::SectorStorageError;

    switch (e) {
        case (SectorStorageError::CANNOT_CREATE_DIR):
            return "Sector Storage: cannot create a directory";
        default:
            return "Sector Storage: unknown error";
    }
}
