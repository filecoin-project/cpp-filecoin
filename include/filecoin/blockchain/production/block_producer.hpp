/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/common/outcome.hpp"
#include "filecoin/primitives/address/address.hpp"
#include "filecoin/primitives/block/block.hpp"
#include "filecoin/primitives/ticket/epost_ticket.hpp"
#include "filecoin/primitives/ticket/ticket.hpp"
#include "filecoin/vm/indices/indices.hpp"

namespace fc::blockchain::production {
  /**
   * @class Generating new blocks for chain's tipset
   */
  class BlockProducer {
   protected:
    using Block = primitives::block::Block;
    using Address = primitives::address::Address;
    using EPostProof = primitives::ticket::EPostProof;
    using Ticket = primitives::ticket::Ticket;
    using Indices = vm::indices::Indices;

   public:
    virtual ~BlockProducer() = default;

    /**
     * @brief Generate new block
     * @param miner_address - source address
     * @param parent_tipset_id - id of the parent tipset
     * @param proof - evidence of mining permission
     * @param ticket - used ticket for election round
     * @return Generated full block
     */
    virtual outcome::result<Block> generate(
        Address miner_address,
        const CID &parent_tipset_id,
        EPostProof proof,
        Ticket ticket,
        std::shared_ptr<Indices> indices) = 0;
  };

  /**
   * @enum Block production errors
   */
  enum class BlockProducerError {
    PARENT_TIPSET_NOT_FOUND = 1,
    PARENT_TIPSET_INVALID_CONTENT
  };
}  // namespace fc::blockchain::production

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::production, BlockProducerError);
