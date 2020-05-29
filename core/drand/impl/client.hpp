/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_IMPL_CLIENT_HPP
#define CPP_FILECOIN_CORE_DRAND_IMPL_CLIENT_HPP

#include <grpc++/grpc++.h>
#include <boost/optional.hpp>
#include "common/outcome.hpp"
#include "drand/client.hpp"
#include "drand/protobuf/api.grpc.pb.h"
#include "drand/protobuf/api.pb.h"

namespace fc::drand {

  class DrandSyncClientImpl : public DrandSyncClient {
   public:
    enum class Error {
      GRPC_FAILURE = 1,
    };
    DrandSyncClientImpl(
        std::string host,
        size_t port,
        boost::optional<std::string> pem_root_certs = boost::none);

    outcome::result<PublicRandResponse> publicRand(uint64_t round) override;

    outcome::result<std::vector<PublicRandResponse>> publicRandStream(
        uint64_t round) override;

    outcome::result<GroupPacket> group() override;

   private:
    std::string host_;
    size_t port_;
    std::unique_ptr<::drand::Public::StubInterface> stub_;
  };
}  // namespace fc::drand

OUTCOME_HPP_DECLARE_ERROR(fc::drand, DrandSyncClientImpl::Error);

#endif  // CPP_FILECOIN_CORE_DRAND_IMPL_CLIENT_HPP
