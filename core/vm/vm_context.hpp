/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_CONTEXT_HPP
#define CPP_FILECOIN_CORE_VM_CONTEXT_HPP

namespace fc::vm {

  // TODO(artyom-yurin): it is temp structure, will be removed after
  // "runtime" would be implemented
  struct Message {
    primitives::address::Address from;
  };

  class VMContext {
   public:
    virtual ~VMContext() = default;

    virtual outcome::result<void> send(primitives::address::Address to,
                                       uint64_t method,
                                       actor::BigInt value,
                                       std::vector<uint8_t> params) = 0;
    virtual Message message() = 0;
  };
}  // namespace fc::vm

#endif  // CPP_FILECOIN_CORE_VM_CONTEXT_HPP
