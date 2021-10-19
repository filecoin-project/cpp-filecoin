/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/uvarint_key.hpp"
#include "codec/cbor/streams_annotation.hpp"

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"

// Forward declaration
namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::types::multisig {
  using primitives::TokenAmount;
  using primitives::address::Address;

  using TransactionId = int64_t;
  using TransactionKeyer = adt::VarintKeyer;

  /**
   * Multisignaure pending transaction
   */
  struct Transaction {
    Address to;
    TokenAmount value{};
    MethodNumber method{};
    MethodParams params;

    /**
     * @brief List of addresses that approved transaction
     * This address at index 0 is the transaction proposer, order of this
     * slice must be preserved
     */
    std::vector<Address> approved;

    inline bool operator==(const Transaction &other) const {
      return to == other.to && value == other.value && method == other.method
             && params == other.params && approved == other.approved;
    }

    outcome::result<Bytes> hash(fc::vm::runtime::Runtime &runtime) const;
  };
  CBOR_TUPLE(Transaction, to, value, method, params, approved)

  /**
   * Data for a BLAKE2B-256 to be attached to methods referencing proposals
   * via TXIDs. Ensures the existence of a cryptographic reference to the
   * original proposal. Useful for offline signers and for protection when
   * reorgs change a multisig TXID.
   */
  struct ProposalHashData {
    Address requester;
    Address to;
    TokenAmount value{};
    MethodNumber method{};
    MethodParams params;

    explicit ProposalHashData(const Transaction &transaction)
        : requester(!transaction.approved.empty() ? transaction.approved[0]
                                                  : Address{}),
          to(transaction.to),
          value(transaction.value),
          method(transaction.method),
          params(transaction.params) {}
  };
  CBOR_TUPLE(ProposalHashData, requester, to, value, method, params)

}  // namespace fc::vm::actor::builtin::types::multisig
