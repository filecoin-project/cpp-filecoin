/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <libp2p/multi/content_identifier_codec.hpp>
#include "blockchain/message_pool/message_storage.hpp"
#include "blockchain/production/impl/block_producer_impl.hpp"
#include "clock/impl/chain_epoch_clock_impl.hpp"
#include "clock/time.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/tipset/tipset.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/blockchain/message_pool/message_storage_mock.hpp"
#include "testutil/mocks/blockchain/weight_calculator_mock.hpp"
#include "testutil/mocks/clock/utc_clock_mock.hpp"
#include "testutil/mocks/crypto/bls/bls_provider_mock.hpp"
#include "testutil/mocks/storage/ipfs/ipfs_datastore_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/mocks/vm/interpreter/interpreter_mock.hpp"
#include "testutil/outcome.hpp"

namespace config {
  constexpr size_t kMinerAddressId{32615184};
  constexpr size_t kGenesisTime{7000};
  constexpr size_t kBlockCreationUnixTime{48151623};
  constexpr size_t kParentTipsetWeight{111307};
  const auto kParentTipset{"010001020005"_cid};
  const std::vector<fc::CID> kParentTipsetBlocks{
      "010001020006"_cid,
      "010001020007"_cid};
  const fc::common::Buffer kPostProof{"a0b0cc"_unhex};
  const auto kPostRand{
      "e9cecfc7c4c120d4c1cb20c8cfdec5d4d3d120dac1c2d9d4d820cf20d1ddc9cbc520d320"
      "cdc9cbd2cfd3c8c5cdc1cdc920c920cec520cfd4cbd2d9d7c1d4d820d7cfccdbc5c2ced9"
      "c520d7cfd2cfd4c120d720d7cfccdbc5c2ced9ca20cdc9d2"_blob96};
  const auto kTicket{
      "7672662070726f6f66303030303030307672662070726f6f663030303030303076726620"
      "70726f6f66303030303030307672662070726f6f66303030303030307672662070726f6f"
      "66303030303030307672662070726f6f6630303030303030"_blob96};
}  // namespace config

class BlockProducerTest : public testing::Test {
 protected:
  using CID = fc::CID;
  using UTCClock = fc::clock::UTCClock;
  using UTCClockImpl = UTCClockMock;
  using ChainEpochClock = fc::clock::ChainEpochClock;
  using ChainEpochClockImpl = fc::clock::ChainEpochClockImpl;
  using Time = fc::clock::Time;
  using Address = fc::primitives::address::Address;
  using Tipset = fc::primitives::tipset::Tipset;
  using IpfsDatastore = fc::storage::ipfs::IpfsDatastore;
  using IpfsDatastoreImpl = fc::storage::ipfs::MockIpfsDatastore;
  using MessageStorage = fc::blockchain::message_pool::MessageStorage;
  using MessageStorageImpl = fc::blockchain::message_pool::MessageStorageMock;
  using UnsignedMessage = fc::primitives::block::UnsignedMessage;
  using SignedMessage = fc::blockchain::message_pool::SignedMessage;
  using BlsSignature = fc::crypto::signature::BlsSignature;
  using SecpSignature = fc::crypto::signature::Secp256k1Signature;
  using WeightCalculator = fc::blockchain::weight::WeightCalculator;
  using WeightCalculatorImpl = fc::blockchain::weight::WeightCalculatorMock;
  using BlsProvider = fc::crypto::bls::BlsProvider;
  using BlsProviderImpl = fc::crypto::bls::BlsProviderMock;
  using Interpreter = fc::vm::interpreter::Interpreter;
  using InterpreterImpl = fc::vm::interpreter::InterpreterMock;
  using CIDCodec = libp2p::multi::ContentIdentifierCodec;
  using EPostProof = fc::primitives::ticket::EPostProof;
  using PostRandomness = fc::primitives::ticket::PostRandomness;
  using Ticket = fc::primitives::ticket::Ticket;
  using Indices = fc::vm::indices::Indices;
  using IndicesImpl = fc::vm::indices::MockIndices;
  using BlockProducer = fc::blockchain::production::BlockProducer;
  using BlockProducerImpl = fc::blockchain::production::BlockProducerImpl;

  EPostProof e_post_proof_{.proof = config::kPostProof,
                           .post_rand = config::kPostRand,
                           .candidates = {}};

  Tipset getParentTipset() const;

  Ticket getTicket() const;

  std::vector<SignedMessage> getSampleMessages() const;
};
