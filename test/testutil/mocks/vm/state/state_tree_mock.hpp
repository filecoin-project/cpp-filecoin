/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_MOCK_HPP
#define CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_MOCK_HPP

#include <gmock/gmock.h>
#include "vm/state/state_tree.hpp"

namespace fc::vm::state {

  class MockStateTree : public StateTree {
   public:
    MOCK_METHOD2(set,
                 outcome::result<void>(const Address &address,
                                       const Actor &actor));
    MOCK_METHOD1(get, outcome::result<Actor>(const Address &address));
    MOCK_METHOD1(lookupId, outcome::result<Address>(const Address &address));
    MOCK_METHOD2(registerNewAddress,
                 outcome::result<Address>(const Address &address,
                                          const Actor &actor));
    MOCK_METHOD0(flush, outcome::result<CID>());
    MOCK_METHOD0(revert, outcome::result<void>());
    MOCK_METHOD0(getStore, std::shared_ptr<IpfsDatastore>());
  };

}  // namespace fc::vm::state

#endif  // CPP_FILECOIN_CORE_VM_STATE_STATE_TREE_MOCK_HPP
