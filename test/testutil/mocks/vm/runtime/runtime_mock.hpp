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
    MOCK_CONST_METHOD0(execution, std::shared_ptr<Execution>());

    MOCK_CONST_METHOD0(getNetworkVersion, NetworkVersion());

    MOCK_CONST_METHOD0(getCurrentEpoch, ChainEpoch());

    MOCK_CONST_METHOD3(
        getRandomnessFromTickets,
        outcome::result<Randomness>(DomainSeparationTag tag,
                                    ChainEpoch epoch,
                                    gsl::span<const uint8_t> seed));

    MOCK_CONST_METHOD3(
        getRandomnessFromBeacon,
        outcome::result<Randomness>(DomainSeparationTag tag,
                                    ChainEpoch epoch,
                                    gsl::span<const uint8_t> seed));

    MOCK_CONST_METHOD0(getImmediateCaller, Address());

    MOCK_CONST_METHOD0(getCurrentReceiver, Address());

    MOCK_CONST_METHOD1(getBalance,
                       outcome::result<BigInt>(const Address &address));
    MOCK_CONST_METHOD0(getValueReceived, BigInt());

    MOCK_CONST_METHOD1(getActorCodeID,
                       outcome::result<CodeId>(const Address &address));

    MOCK_METHOD4(send,
                 outcome::result<InvocationOutput>(Address to_address,
                                                   MethodNumber method_number,
                                                   MethodParams params,
                                                   BigInt value));

    MOCK_METHOD0(createNewActorAddress, outcome::result<Address>());

    MOCK_METHOD2(createActor,
                 outcome::result<void>(const Address &address,
                                       const Actor &actor));

    MOCK_METHOD1(deleteActor, outcome::result<void>(const Address &address));

    MOCK_METHOD3(transfer,
                 outcome::result<void>(const Address &debitFrom,
                                       const Address &creditTo,
                                       const TokenAmount &amount));

    MOCK_CONST_METHOD0(getTotalFilCirculationSupply,
                       fc::outcome::result<TokenAmount>());

    MOCK_METHOD0(getIpfsDatastore, std::shared_ptr<IpfsDatastore>());

    MOCK_METHOD0(getMessage, std::reference_wrapper<const UnsignedMessage>());

    MOCK_METHOD1(chargeGas, outcome::result<void>(GasAmount amount));

    MOCK_METHOD0(getCurrentActorState, outcome::result<CID>());

    MOCK_METHOD1(commit, outcome::result<void>(const CID &new_state));

    MOCK_METHOD1(resolveAddress,
                 outcome::result<Address>(const Address &address));

    MOCK_METHOD3(verifySignature,
                 outcome::result<bool>(const Signature &signature,
                                       const Address &address,
                                       gsl::span<const uint8_t> data));

    MOCK_METHOD3(verifySignatureBytes,
                 outcome::result<bool>(const Buffer &signature_bytes,
                                       const Address &address,
                                       gsl::span<const uint8_t> data));

    MOCK_METHOD1(verifyPoSt,
                 outcome::result<bool>(const WindowPoStVerifyInfo &info));

    MOCK_METHOD2(computeUnsealedSectorCid,
                 outcome::result<CID>(RegisteredProof,
                                      const std::vector<PieceInfo> &));

    MOCK_METHOD3(verifyConsensusFault,
                 outcome::result<ConsensusFault>(const Buffer &block1,
                                                 const Buffer &block2,
                                                 const Buffer &extra));

    /// Expect call to send with params returing result
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
