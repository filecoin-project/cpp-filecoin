/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_IMPL_BLOCK_VALIDATOR_IMPL_HPP
#define CPP_FILECOIN_CORE_CHAIN_IMPL_BLOCK_VALIDATOR_IMPL_HPP

#include <functional>
#include <map>
#include <memory>

#include <boost/optional.hpp>
#include <libp2p/crypto/secp256k1_provider.hpp>
#include "blockchain/block_validator/block_validator.hpp"
#include "blockchain/weight_calculator.hpp"
#include "clock/chain_epoch_clock.hpp"
#include "clock/utc_clock.hpp"
#include "crypto/bls/bls_provider.hpp"
#include "power/power_table.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/indices/indices.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::blockchain::block_validator {

  class BlockValidatorImpl : public BlockValidator {
   protected:
    using Stage = scenarios::Stage;
    using IpfsDatastore = storage::ipfs::IpfsDatastore;
    using EpochClock = clock::ChainEpochClock;
    using UTCClock = clock::UTCClock;
    using WeightCalculator = blockchain::weight::WeightCalculator;
    using PowerTable = power::PowerTable;
    using BlsProvider = crypto::bls::BlsProvider;
    using SecpProvider = libp2p::crypto::secp256k1::Secp256k1Provider;
    using Interpreter = vm::interpreter::Interpreter;
    using Tipset = primitives::tipset::Tipset;
    using Indices = vm::indices::Indices;

   public:
    using StageExecutor = outcome::result<void> (BlockValidatorImpl::*)(
        const BlockHeader &) const;

    BlockValidatorImpl(std::shared_ptr<IpfsDatastore> ipfs_store,
                       std::shared_ptr<UTCClock> utc_clock,
                       std::shared_ptr<EpochClock> epoch_clock,
                       std::shared_ptr<WeightCalculator> weight_calculator,
                       std::shared_ptr<PowerTable> power_table,
                       std::shared_ptr<BlsProvider> bls_crypto_provider,
                       std::shared_ptr<SecpProvider> secp_crypto_provider,
                       std::shared_ptr<Interpreter> vm_interpreter,
                       std::shared_ptr<Indices> indices)
        : datastore_{std::move(ipfs_store)},
          clock_{std::move(utc_clock)},
          epoch_clock_{std::move(epoch_clock)},
          weight_calculator_{std::move(weight_calculator)},
          power_table_{std::move(power_table)},
          bls_provider_{std::move(bls_crypto_provider)},
          secp_provider_{std::move(secp_crypto_provider)},
          vm_interpreter_{std::move(vm_interpreter)},
          vm_indices_{std::move(indices)} {}

    outcome::result<void> validateBlock(
        const BlockHeader &header, scenarios::Scenario scenario) const override;

   private:
    const static std::map<scenarios::Stage, StageExecutor> stage_executors_;

    std::shared_ptr<IpfsDatastore> datastore_;
    std::shared_ptr<UTCClock> clock_;
    std::shared_ptr<EpochClock> epoch_clock_;
    std::shared_ptr<WeightCalculator> weight_calculator_;
    std::shared_ptr<PowerTable> power_table_;
    std::shared_ptr<BlsProvider> bls_provider_;
    std::shared_ptr<SecpProvider> secp_provider_;
    std::shared_ptr<Interpreter> vm_interpreter_;
    std::shared_ptr<Indices> vm_indices_;

    /**
     * BlockHeader CID -> Parent tipset
     * Assuming that block validation executes sequentially,
     * so for several blocks parent tipset will be the same
     */
    mutable boost::optional<std::pair<CID, Tipset>> parent_tipset_cache_;

    /**
     * @brief Check block syntax
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> syntax(const BlockHeader &header) const;

    /**
     * @brief Check consensus rules
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> consensus(const BlockHeader &header) const;

    /**
     * @brief Check block signature
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> blockSign(const BlockHeader &header) const;

    /**
     * @brief Check miner election params
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> electionPost(const BlockHeader &header) const;

    /**
     * @brief Check chain ancestry params
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> chainAncestry(const BlockHeader &header) const;

    /**
     * @brief Check block messages signatures
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> messageSign(const BlockHeader &header) const;

    /**
     * @brief Check parent state tree
     * @param header - block to check
     * @return Check result
     */
    outcome::result<void> stateTree(const BlockHeader &header) const;

    /**
     * @brief Load parent tipset for selected block
     * @param header - selected block
     * @return operation result
     */
    outcome::result<std::reference_wrapper<Tipset>> getParentTipset(
        const BlockHeader &header) const;
  };

  enum class ValidatorError {
    UNKNOWN_STAGE = 1,
    UNKNOWN_BLOCK_SIGNATURE,
    INVALID_BLOCK_SIGNATURE,
    INVALID_MINER_PUBLIC_KEY,
    INVALID_PARENT_STATE,
  };

}  // namespace fc::blockchain::block_validator

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::block_validator, ValidatorError);

#endif  // CPP_FILECOIN_CORE_CHAIN_IMPL_BLOCK_VALIDATOR_IMPL_HPP
