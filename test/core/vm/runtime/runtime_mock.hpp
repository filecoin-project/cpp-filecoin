/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_RUNTIME_MOCK_HPP
#define CPP_FILECOIN_RUNTIME_MOCK_HPP

#include <gmock/gmock.h>
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

    MOCK_METHOD0(returnSuccess, InvocationOutput());
    MOCK_METHOD1(returnValue, InvocationOutput(Buffer bytes));

    MOCK_METHOD2(abort,
                 void(const ExitCode &errExitCode, const std::string &msg));

    MOCK_METHOD1(abortArg, void(const std::string &message));
    MOCK_METHOD0(abortArg, void());

    MOCK_METHOD1(abortState, void(const std::string &message));
    MOCK_METHOD0(abortState, void());

    MOCK_METHOD1(abortFunds, void(const std::string &message));
    MOCK_METHOD0(abortFunds, void());

    MOCK_METHOD1(abortAPI, void(const std::string &message));

    // TODO(a.chernyshov) assert
    //    virtual void assert(bool condition) = 0;

    MOCK_CONST_METHOD1(getBalance,
                       outcome::result<BigInt>(const Address &address));
    MOCK_CONST_METHOD0(getValueReceived, BigInt());

    MOCK_CONST_METHOD0(getCurrentIndices, std::shared_ptr<Indices>());

    MOCK_CONST_METHOD1(getActorCodeID,
                       outcome::result<CodeId>(const Address &address));

    // TODO(a.chernyshov) util.Any type
    // Compute(ComputeFunctionID, args [util.Any]) util.Any
    MOCK_METHOD4(send,
                 outcome::result<InvocationOutput>(Address to_address,
                                                   MethodNumber method_number,
                                                   MethodParams params,
                                                   BigInt value));

    MOCK_METHOD0(createNewActorAddress, Address());

    MOCK_METHOD2(createActor,
                 outcome::result<void>(CodeId code_id, const Address &address));

    MOCK_METHOD1(deleteActor, outcome::result<void>(const Address &address));

    MOCK_METHOD0(getIpfsDatastore, std::shared_ptr<IpfsDatastore>());

    MOCK_METHOD0(getMessage, std::shared_ptr<UnsignedMessage>());
  };
}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_RUNTIME_MOCK_HPP
