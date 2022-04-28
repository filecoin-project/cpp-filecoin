/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utf8/utf8/core.h>
#include <boost/variant.hpp>

#include "codec/cbor/cbor_codec.hpp"
#include "common/error_text.hpp"
#include "common/visitor.hpp"
#include "primitives/address/address.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::market {
  using codec::cbor::CborDecodeError;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  /**
   * According FIP-0027 label is a string or bytes from actors v8.
   */
  class Label : public boost::variant<std::string, Bytes> {
   public:
    Label() : boost::variant<std::string, Bytes>("") {}

    static outcome::result<Label> make(std::string str) {
      if (not utf8::is_valid(str.begin(), str.end())) {
        return ERROR_TEXT("Label must be a valid UTF-8 or byte array.");
      }
      return Label{std::move(str)};
    }

    static outcome::result<Label> make(Bytes bytes) {
      return Label{std::move(bytes)};
    }

    inline size_t length() const {
      return visit_in_place(
          *this,
          [&](const std::string &str) { return str.length(); },
          [&](const Bytes &bytes) { return bytes.size(); });
    }

   private:
    explicit Label(std::string str)
        : boost::variant<std::string, Bytes>(std::move(str)) {}

    explicit Label(Bytes bytes)
        : boost::variant<std::string, Bytes>(std::move(bytes)) {}
  };

  inline CBOR2_ENCODE(Label) {
    visit_in_place(
        v,
        [&](const std::string &str) { s << str; },
        [&](const Bytes &bytes) { s << bytes; });
    return s;
  }

  inline CBOR2_DECODE(Label) {
    if (s.isStr()) {
      std::string str;
      s >> str;
      v = Label::make(str).value();
      return s;
    }
    if (s.isBytes()) {
      Bytes bytes;
      s >> bytes;
      v = Label::make(bytes).value();
      return s;
    }
    // must be a string or bytes
    outcome::raise(CborDecodeError::kInvalidCbor);
    return s;
  }

  struct DealProposal {
    virtual ~DealProposal() = default;

    inline TokenAmount clientBalanceRequirement() const {
      return client_collateral + getTotalStorageFee();
    }

    inline TokenAmount providerBalanceRequirement() const {
      return provider_collateral;
    }

    inline EpochDuration duration() const {
      return end_epoch - start_epoch;
    }

    inline TokenAmount getTotalStorageFee() const {
      return storage_price_per_epoch * duration();
    }

    virtual CID cid() const = 0;

    CID piece_cid;
    PaddedPieceSize piece_size;
    bool verified = false;
    Address client;
    Address provider;

    virtual size_t getLabelLength() const = 0;

    // FIP-0027 introduced Label type in order to support non UTF-8 labels as
    // raw bytes while UTF-8 labels are encoded as strings
    std::string label_v0;
    Label label_v8;

    ChainEpoch start_epoch{};
    ChainEpoch end_epoch{};
    TokenAmount storage_price_per_epoch;
    TokenAmount provider_collateral;
    TokenAmount client_collateral;

    inline bool operator==(const DealProposal &other) const {
      return piece_cid == other.piece_cid && piece_size == other.piece_size
             && client == other.client && provider == other.provider
             && start_epoch == other.start_epoch && end_epoch == other.end_epoch
             && storage_price_per_epoch == other.storage_price_per_epoch
             && provider_collateral == other.provider_collateral
             && client_collateral == other.client_collateral;
    }

    inline bool operator!=(const DealProposal &other) const {
      return !(*this == other);
    }
  };

}  // namespace fc::vm::actor::builtin::types::market
