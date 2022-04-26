/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "crypto/hasher/hasher.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/builtin/types/market/deal_proposal.hpp"

namespace fc::vm::actor::builtin::v0::market {
  using crypto::Hasher;

  struct DealProposal : public types::market::DealProposal {
    inline CID cid() const override {
      OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
      return {CID::Version::V1,
              CID::Multicodec::DAG_CBOR,
              Hasher::blake2b_256(bytes)};
    }

    size_t getLabelLength() const override {
      return label_v0.length();
    }
  };

  CBOR_TUPLE(DealProposal,
             piece_cid,
             piece_size,
             verified,
             client,
             provider,
             label_v0,
             start_epoch,
             end_epoch,
             storage_price_per_epoch,
             provider_collateral,
             client_collateral)
}  // namespace fc::vm::actor::builtin::v0::market