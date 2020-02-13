/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP

#include "storage/ipfs/datastore.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::vm::interpreter {
  using primitives::tipset::Tipset;
  using storage::ipfs::IpfsDatastore;

  struct Result {
    CID state_root;
    CID message_receipts;
  };

  outcome::result<Result> interpret(const std::shared_ptr<IpfsDatastore> &store, const Tipset &tipset);
}  // namespace fc::vm::interpreter

#endif  // CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
