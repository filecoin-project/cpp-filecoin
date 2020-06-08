/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/network/sync_cbor_stream.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::retrieval::network,
                            SyncCborStreamErrors,
                            e) {
  using fc::markets::retrieval::network::SyncCborStreamErrors;
  switch (e) {
    case SyncCborStreamErrors::WRITE_ERROR:
      return "Sync CBOR stream write error";
    case SyncCborStreamErrors::READ_ERROR:
      return "Sync CBOR stream read error";
  }
}  // namespace fc::markets::retrieval::network
