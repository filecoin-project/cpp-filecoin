/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPLD_VERIFIER_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPLD_VERIFIER_HPP

#include "common/buffer.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/ipld/traverser.hpp"

namespace fc::storage::ipld::verifier {
  using common::Buffer;
  using ipfs::InMemoryDatastore;
  using traverser::Traverser;

  /**
   * @brief Type of errors returned by IPLD traverser
   */
  enum class VerifierError {
    kUnexpectedCid = 1,
  };

  /**
   * Verifies IPLD blocks by traversing one by one
   */
  class Verifier {
   public:
    /**
     * @param root - payload root cid
     * @param selector - selector
     */
    Verifier(const CID &root, const Selector &selector);

    /**
     * Try apply next block
     * @param block_cid - to apply
     * @param block_data - block payload
     * @return traversal completed - true if blocks are not expected anymore
     */
    outcome::result<bool> verifyNextBlock(const CID &block_cid,
                                          const Buffer &block_data);

   private:
    InMemoryDatastore store_;
    Traverser traverser_;
  };

}  // namespace fc::storage::ipld::verifier

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipld::verifier, VerifierError);

#endif  // CPP_FILECOIN_CORE_STORAGE_IPLD_VERIFIER_HPP
