/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::markets::storage {

  /**
   * Unique string identifier for a StorageDataTransferVoucher
   */
  const std::string StorageDataTransferVoucherType =
      "StorageDataTransferVoucher";

  /**
   * Datatransfer voucher data
   */
  struct StorageDataTransferVoucher {
    CID proposal_cid;
  };

  CBOR_TUPLE(StorageDataTransferVoucher, proposal_cid);

}  // namespace fc::markets::storage
