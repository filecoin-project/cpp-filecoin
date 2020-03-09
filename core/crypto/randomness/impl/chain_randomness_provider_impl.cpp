/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/crypto/randomness/impl/chain_randomness_provider_impl.hpp"

#include <boost/assert.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include "filecoin/common/le_encoder.hpp"
#include "filecoin/primitives/ticket/ticket.hpp"
#include "filecoin/primitives/tipset/tipset_key.hpp"
#include "filecoin/storage/chain/chain_store.hpp"

namespace fc::crypto::randomness {

  using crypto::randomness::Randomness;
  using primitives::ticket::Ticket;
  using primitives::tipset::TipsetKey;

  namespace {
    /**
     * @brief ChainStore needs its own randomnes calculation function
     * @return randomness value
     */
    crypto::randomness::Randomness drawRandomness(const Ticket &ticket,
                                                  uint64_t round) {
      common::Buffer buffer{};
      const size_t bytes_required = sizeof(round) + ticket.bytes.size();
      buffer.reserve(bytes_required);

      buffer.put(ticket.bytes);
      common::encodeLebInteger(round, buffer);

      auto hash = libp2p::crypto::sha256(buffer);

      return crypto::randomness::Randomness(hash);
    }
  }  // namespace

  ChainRandomnessProviderImpl::ChainRandomnessProviderImpl(
      std::shared_ptr<storage::blockchain::ChainStore> chain_store)
      : chain_store_(std::move(chain_store)) {
    BOOST_ASSERT_MSG(chain_store_ != nullptr,
                     "chain_store argument == nullptr");
  }

  outcome::result<Randomness> ChainRandomnessProviderImpl::sampleRandomness(
      const std::vector<CID> &block_cids, uint64_t round) {
    std::reference_wrapper<const std::vector<CID>> cids{block_cids};

    while (true) {
      OUTCOME_TRY(tipset, chain_store_->loadTipset(TipsetKey{cids.get()}));
      OUTCOME_TRY(min_ticket_block, tipset.getMinTicketBlock());

      if (tipset.height <= round) {
        BOOST_ASSERT_MSG(min_ticket_block.get().ticket.has_value(),
                         "min ticket block has no value, internal error");

        return drawRandomness(*min_ticket_block.get().ticket, round);
      }

      // special case for lookback behind genesis block
      if (min_ticket_block.get().height == 0) {
        // round is negative
        auto &&negative_hash =
            drawRandomness(*min_ticket_block.get().ticket, round - 1);
        // for negative lookbacks, just use the hash of the positive ticket hash
        // value
        auto &&positive_hash = libp2p::crypto::sha256(negative_hash);
        return Randomness{positive_hash};
      }

      // TODO (yuraz) I know it's very ugly, and needs to be refactored
      // I translated it directly from go to C++
      cids = min_ticket_block.get().parents;
    }
  }

}  // namespace fc::crypto::randomness
