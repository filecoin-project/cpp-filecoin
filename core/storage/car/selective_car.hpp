/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CAR_SELECTIVE_CAR_HPP
#define CPP_FILECOIN_CORE_STORAGE_CAR_SELECTIVE_CAR_HPP

#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/ipfs/merkledag/merkledag_service.hpp"
#include "storage/ipld/ipld_node.hpp"

namespace fc::storage::car {

  using common::Buffer;
  using fc::storage::ipfs::merkledag::MerkleDagService;
  using storage::ipld::IPLDNode;

  /**
   * SelectiveCar is a car file based on root + selector combos
   */

  /**
   * Dag is a root/selector combo to put into a car
   */
  struct DAG {
    CID root;
    std::shared_ptr<IPLDNode> selector;
  };

  /**
   * Block is all information and metadata about a block that is part of a car
   * file
   */
  struct SelectiveCarBlock {
    CID block_cid;
    Buffer data;
    size_t offset;
    size_t size;
  };

  outcome::result<Buffer> makeSelectiveCar(
      std::shared_ptr<MerkleDagService> dag_service,
      const std::vector<DAG> &dags);

}  // namespace fc::storage::car

#endif  // CPP_FILECOIN_CORE_STORAGE_CAR_SELECTIVE_CAR_HPP
