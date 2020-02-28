/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_RUNTIME_MOCK_HPP
#define CPP_FILECOIN_RUNTIME_MOCK_HPP

#include <gmock/gmock.h>

#include "testutil/outcome.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::runtime {

  class MockRuntime : public Runtime {
   public:
    MOCK_CONST_METHOD0(getCurrentEpoch, ChainEpoch());

    MOCK_CONST_METHOD2(getRandomness,
                       Randomness(DomainSeparationTag tag, ChainEpoch epoch));
    MOCK_CONST_METHOD3(getRandomness,
                       Randomness(DomainSeparationTag tag,
                                  ChainEpoch epoch,
                                  Serialization seed));

    MOCK_CONST_METHOD0(getImmediateCaller, Address());

    MOCK_CONST_METHOD0(getCurrentReceiver, Address());

    MOCK_CONST_METHOD0(getTopLevelBlockWinner, Address());

    MOCK_CONST_METHOD0(acquireState, std::shared_ptr<ActorStateHandle>());

    MOCK_CONST_METHOD1(getBalance,
                       outcome::result<BigInt>(const Address &address));
    MOCK_CONST_METHOD0(getValueReceived, BigInt());

    MOCK_CONST_METHOD0(getCurrentIndices, std::shared_ptr<Indices>());

    MOCK_CONST_METHOD1(getActorCodeID,
                       outcome::result<CodeId>(const Address &address));

    MOCK_METHOD4(send,
                 outcome::result<InvocationOutput>(Address to_address,
                                                   MethodNumber method_number,
                                                   MethodParams params,
                                                   BigInt value));

    MOCK_METHOD0(createNewActorAddress, Address());

    MOCK_METHOD2(createActor,
                 outcome::result<void>(const Address &address,
                                       const Actor &actor));

    MOCK_METHOD0(deleteActor, outcome::result<void>());

    MOCK_METHOD0(getIpfsDatastore, std::shared_ptr<IpfsDatastore>());

    MOCK_METHOD0(getMessage, std::reference_wrapper<const UnsignedMessage>());

    MOCK_METHOD1(chargeGas, outcome::result<void>(const BigInt &amount));

    MOCK_METHOD0(getCurrentActorState, ActorSubstateCID());

    MOCK_METHOD1(commit,
                 outcome::result<void>(const ActorSubstateCID &new_state));

    MOCK_METHOD1(resolveAddress,
                 outcome::result<Address>(const Address &address));

    MOCK_METHOD2(verifyPoSt,
                 outcome::result<bool>(uint64_t sector_size,
                                       const PoStVerifyInfo &info));

    MOCK_METHOD2(verifySeal,
                 outcome::result<bool>(uint64_t sector_size,
                                       const SealVerifyInfo &info));

    MOCK_METHOD2(verifyConsensusFault,
                 outcome::result<bool>(const BlockHeader &block_header_1,
                                       const BlockHeader &block_header_2));

    template <typename M>
    void expectSendM(const Address &address,
                     const typename M::Params &params,
                     TokenAmount value,
                     const typename M::Result &result) {
      EXPECT_OUTCOME_TRUE(params2, actor::encodeActorParams(params));
      EXPECT_OUTCOME_TRUE(result2, actor::encodeActorReturn(result));
      EXPECT_CALL(*this, send(address, M::Number, params2, value))
          .WillOnce(testing::Return(fc::outcome::success(result2)));
    }
  };
}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_RUNTIME_MOCK_HPP
