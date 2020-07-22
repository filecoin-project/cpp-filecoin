/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/verifier.hpp"

namespace fc::storage::ipld::verifier {

  Verifier::Verifier(const CID &root, const Selector &selector)
      : traverser_{store_, root, selector} {}

  outcome::result<bool> Verifier::verifyNextBlock(const CID &block_cid,
                                                  const Buffer &data) {
    OUTCOME_TRY(data_cid, common::getCidOf(data));
    if (block_cid != data_cid) {
      return VerifierError::kUnexpectedCid;
    }
    OUTCOME_TRY(store_.set(block_cid, data));
    OUTCOME_TRY(traversed_cid, traverser_.advance());
    if (block_cid != traversed_cid) {
      return VerifierError::kUnexpectedCid;
    }
    return traverser_.isCompleted();
  }

}  // namespace fc::storage::ipld::verifier

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipld::verifier, VerifierError, e) {
  using fc::storage::ipld::verifier::VerifierError;

  switch (e) {
    case VerifierError::kUnexpectedCid:
      return "IPLD Block Verifier: unexpected CID encountered";
    default:
      return "IPLD Block Verifier: unknown error";
  }
}
