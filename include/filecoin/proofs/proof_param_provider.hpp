/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PROOF_PARAM_PROVIDER_HPP
#define CPP_FILECOIN_PROOF_PARAM_PROVIDER_HPP

#include "filecoin/common/logger.hpp"
#include "filecoin/common/outcome.hpp"
#include "gsl/span"

namespace fc::proofs {

  struct ParamFile {
    std::string name;
    std::string cid;
    std::string digest;
    uint64_t sector_size = 0;
  };

  class ProofParamProvider {
   public:
    static outcome::result<void> getParams(
        const gsl::span<ParamFile> &param_files, uint64_t storage_size);

    static outcome::result<std::vector<ParamFile>> readJson(
        const std::string &path);

   private:
    static void fetch(const ParamFile &info);
    static outcome::result<void> doFetch(const std::string &out,
                                         const ParamFile &info);

    static common::Logger logger_;
  };

}  // namespace fc::proofs

#endif  // CPP_FILECOIN_PROOF_PARAM_PROVIDER_HPP
