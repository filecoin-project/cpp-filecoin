/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "block_producer_test.hpp"

#include "filecoin/codec/cbor/cbor.hpp"

using testing::_;

BlockProducerTest::Tipset BlockProducerTest::getParentTipset() const {
  return Tipset{.cids = config::kParentTipsetBlocks, .blks = {}, .height = 0};
}

BlockProducerTest::Ticket BlockProducerTest::getTicket() const {
  fc::crypto::vrf::VRFProof ticket_proof{};
  std::copy_n(
      config::kTicket.begin(), ticket_proof.size(), ticket_proof.begin());
  return Ticket{.bytes = ticket_proof};
}

std::vector<BlockProducerTest::SignedMessage>
BlockProducerTest::getSampleMessages() const {
  using fc::vm::actor::MethodNumber;
  using fc::vm::actor::MethodParams;
  UnsignedMessage message_A{Address::makeFromId(1),
                            Address::makeFromId(2),
                            123,
                            5,
                            100,
                            1000,
                            MethodNumber{1},
                            MethodParams{}};
  UnsignedMessage message_B{Address::makeFromId(2),
                            Address::makeFromId(3),
                            456,
                            10,
                            120,
                            800,
                            MethodNumber{2},
                            MethodParams{}};
  auto signature_A_bytes{
      "6162636465666768696a6b6c6d6e6f707172737475767778797a6162636465666768696a"
      "6b6c6d6e6f7071"
      "72737475767778797a6162636465666768696a6b6c6d6e6f707172737475767778797a61"
      "62636465666768"
      "696a6b6c6d6e6f707172"_unhex};
  auto signature_B{
      "7271706f6e6d6c6b6a6968676665646362617a797877767574737271706f6e6d6c6b6a69"
      "68676665646362"
      "617a797877767574737271706f6e6d6c6b6a6968676665646362617a7978777675747372"
      "71706f6e6d6c6b"
      "6a696867666564636261"_unhex};
  BlsSignature signature_A{};
  std::copy_n(
      signature_A_bytes.begin(), signature_A_bytes.size(), signature_A.begin());
  return {{message_A, signature_A}, {message_B, signature_B}};
}

/**
 * @given Sample data and required modules
 * @when Generating new block
 * @then Operation must be completed successfully
 */
TEST_F(BlockProducerTest, Main) {
  /**
   * Setup IPFS datastore, which should contain parent tipset
   * BlockProducer will try to obtain serialized parent tipset bytes by CID
   */
  auto parent_tipset = getParentTipset();
  EXPECT_OUTCOME_TRUE(parent_tipset_raw_bytes,
                      fc::codec::cbor::encode(parent_tipset));
  auto ipfs_datastore = std::make_shared<IpfsDatastoreImpl>();
  EXPECT_CALL(*ipfs_datastore, get(config::kParentTipset))
      .WillOnce(testing::Return(fc::outcome::success(parent_tipset_raw_bytes)));

  /**
   * Setup Message Store, which should contain several sample messages
   */
  auto message_store = std::make_shared<MessageStorageImpl>();
  using fc::blockchain::production::config::kBlockMaxMessagesCount;
  EXPECT_CALL(*message_store, getTopScored(kBlockMaxMessagesCount))
      .WillOnce(testing::Return(getSampleMessages()));

  /**
   * Initialize UTC Clock
   */
  auto utc_clock = std::make_shared<UTCClockMock>();
  std::chrono::duration<uint64_t> block_timestamp{
      config::kBlockCreationUnixTime};
  EXPECT_CALL(*utc_clock, nowUTC())
      .Times(1)
      .WillOnce(testing::Return(Time{block_timestamp}));

  /**
   * Initialize Epoch Clock (non-mock implementation)
   */
  std::chrono::duration<uint64_t> genesis_time{config::kGenesisTime};
  auto epoch_clock = std::make_shared<ChainEpochClockImpl>(Time{genesis_time});

  /**
   * Initialize weight calculator
   */
  auto weight_calculator = std::make_shared<WeightCalculatorImpl>();
  EXPECT_CALL(*weight_calculator, calculateWeight(parent_tipset))
      .Times(1)
      .WillOnce(testing::Return(config::kParentTipsetWeight));

  /**
   * Initialize BLS Provider
   */
  auto bls_provider = std::make_shared<BlsProviderImpl>();
  BlsSignature aggregated_signature{};
  auto aggregated_signature_bytes{
      "7271706f6e6d6c6b6a6968676665646362617a797877767574737271706f6e6d6c6b6a69"
      "68676665646362"
      "617a797877767574737271706f6e6d6c6b6a6968676665646362617a7978777675747372"
      "71706f6e6d6c6b"
      "6a696867666564636261"_unhex};
  std::copy_n(aggregated_signature_bytes.begin(),
              aggregated_signature_bytes.size(),
              aggregated_signature.begin());

  EXPECT_CALL(*bls_provider, aggregateSignatures(_))
      .WillOnce(testing::Return(fc::outcome::success(aggregated_signature)));

  /**
   * Initialize VM Interpreter
   */
  using InterpreterResult = fc::vm::interpreter::Result;
  auto vm_interpreter = std::make_shared<InterpreterImpl>();
  std::shared_ptr<Indices> vm_indices = std::make_shared<IndicesImpl>();
  std::shared_ptr<IpfsDatastore> vm_datastore = ipfs_datastore;
  InterpreterResult interpreter_result{config::kParentTipset,
                                       config::kParentTipset};
  EXPECT_CALL(*vm_interpreter, interpret(_, _, _))
      .WillOnce(testing::Return(interpreter_result));

  /**
   * Instantiate Block Producer
   */
  std::shared_ptr<BlockProducer> block_producer =
      std::make_shared<BlockProducerImpl>(ipfs_datastore,
                                          message_store,
                                          utc_clock,
                                          epoch_clock,
                                          weight_calculator,
                                          bls_provider,
                                          vm_interpreter);
  EXPECT_OUTCOME_TRUE_1(
      block_producer->generate(Address::makeFromId(config::kMinerAddressId),
                               config::kParentTipset,
                               e_post_proof_,
                               getTicket(),
                               std::make_shared<IndicesImpl>()));
}
