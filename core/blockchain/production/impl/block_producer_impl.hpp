/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include "blockchain/message_pool/message_storage.hpp"
#include "blockchain/production/block_producer.hpp"
#include "blockchain/weight_calculator.hpp"
#include "clock/chain_epoch_clock.hpp"
#include "clock/utc_clock.hpp"
#include "crypto/bls/bls_provider.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::blockchain::production {
  namespace config {
    // Max messages count in the Block
    constexpr size_t kBlockMaxMessagesCount = 1000;
  }  // namespace config

  /**
   * @class Block Generator implementation
   */
  class BlockProducerImpl : public BlockProducer {
   protected:
    using MessageStorage = blockchain::message_pool::MessageStorage;
    using WeightCalculator = blockchain::weight::WeightCalculator;
    using ChainEpochClock = clock::ChainEpochClock;
    using UTCClock = clock::UTCClock;
    using BlsProvider = crypto::bls::BlsProvider;
    using MsgMeta = primitives::block::MsgMeta;
    using Tipset = primitives::tipset::Tipset;
    using IpfsDatastore = storage::ipfs::IpfsDatastore;
    using SignedMessage = vm::message::SignedMessage;
    using Interpreter = vm::interpreter::Interpreter;

   public:
    /**
     * @brief Construct new Block Generator
     * @param data_store
     */
    explicit BlockProducerImpl(
        std::shared_ptr<IpfsDatastore> data_store,
        std::shared_ptr<MessageStorage> message_store,
        std::shared_ptr<UTCClock> utc_clock,
        std::shared_ptr<ChainEpochClock> epoch_clock,
        std::shared_ptr<WeightCalculator> weight_calculator,
        std::shared_ptr<BlsProvider> crypto_provider,
        std::shared_ptr<Interpreter> interpreter);

    outcome::result<Block> generate(Address miner_address,
                                    const CID &parent_tipset_id,
                                    EPostProof proof,
                                    Ticket ticket,
                                    std::shared_ptr<Indices> indices) override;

   private:
    std::shared_ptr<IpfsDatastore> data_storage_;
    std::shared_ptr<MessageStorage> message_storage_;
    std::shared_ptr<UTCClock> clock_;
    std::shared_ptr<ChainEpochClock> epoch_;
    std::shared_ptr<WeightCalculator> chain_weight_calculator_;
    std::shared_ptr<BlsProvider> bls_provider_;
    std::shared_ptr<Interpreter> vm_interpreter_;

    /**
     * @brief Load tipset from IPFS storage by CID
     * @param tipset_id - CID of the tipset
     * @return requested tipset or appropriate error
     */
    outcome::result<Tipset> getTipset(const CID &tipset_id) const;

    /**
     * @brief Generate messages meta-data
     * @param messages - messages to include in the new Block
     * @return Generated meta-data
     */
    static outcome::result<MsgMeta> getMessagesMeta(
        const std::vector<SignedMessage> &messages);
  };
}  // namespace fc::blockchain::production
