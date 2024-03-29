/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <queue>
#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::storage::ipld::traverser {
  using codec::cbor::CborDecodeStream;

  /**
   * @brief Type of errors returned by IPLD traverser
   */
  enum class TraverserError {
    kTraverseCompleted = 1,
  };

  /**
   * IPLD traverser, stores current traverse state
   */
  class Traverser {
   public:
    /**
     * Constructor with selector
     * @param store - ipld store
     * @param root - root cid
     * @param selector - selector
     * @param unique - should skip duplicates
     */
    Traverser(Ipld &store,
              const CID &root,
              const Selector &selector,
              bool unique);

    /**
     * Traverse all from the root
     * @return all the visited cids
     */
    outcome::result<std::vector<CID>> traverseAll();

    /**
     * Visit only next element
     * Starts with root CID
     * @return cid of traversed block
     */
    outcome::result<CID> advance();

    /**
     * Checks if traversal completed
     * @return true if all cids are visited
     */
    bool isCompleted() const;

   private:
    outcome::result<void> parseCbor(CborDecodeStream &s);

    Ipld &store;
    bool unique{};
    std::vector<CID> to_visit_;     // set of cids to visit
    std::vector<CID> visit_order_;  // visited cids in visit order
    std::set<CID> visited_;         // set of visited cids
  };

}  // namespace fc::storage::ipld::traverser

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipld::traverser, TraverserError);
