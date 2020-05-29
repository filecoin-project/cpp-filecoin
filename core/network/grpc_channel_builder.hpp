/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_NETWORK_GRPC_CHANNEL_BUILDER_HPP
#define CPP_FILECOIN_CORE_NETWORK_GRPC_CHANNEL_BUILDER_HPP

#include <limits>
#include <memory>

#include <grpc++/grpc++.h>
#include <boost/format.hpp>
#include <boost/optional.hpp>

namespace fc::network {

  namespace details {
    constexpr unsigned int kMaxRequestMessageBytes =
        std::numeric_limits<int>::max();
    constexpr unsigned int kMaxResponseMessageBytes =
        std::numeric_limits<int>::max();

    template <typename StubInterface>
    grpc::ChannelArguments getChannelArguments() {
      grpc::ChannelArguments args;
      args.SetServiceConfigJSON((boost::format(R"(
            {
              "methodConfig": [ {
                "name": [
                  { "service": "%1%" }
                ],
                "retryPolicy": {
                  "maxAttempts": 5,
                  "initialBackoff": "5s",
                  "maxBackoff": "120s",
                  "backoffMultiplier": 1.6,
                  "retryableStatusCodes": [
                    "UNKNOWN",
                    "DEADLINE_EXCEEDED",
                    "ABORTED",
                    "INTERNAL"
                  ]
                },
                "maxRequestMessageBytes": %2%,
                "maxResponseMessageBytes": %3%
              } ]
            })") % StubInterface::service_full_name()
                                 % kMaxRequestMessageBytes
                                 % kMaxResponseMessageBytes)
                                    .str());
      return args;
    }
  }  // namespace details

  /**
   * Creates client with specified credentials, which is capable of
   * sending and receiving messages of INT_MAX bytes size with retry policy
   * (see details::getChannelArguments()).
   * @tparam StubInterface gRPC service type
   * @param address where to connect including port number
   * @param chan_creds credentials for the gRPC channel
   * @return gRPC stub of parametrized type
   */
  template <typename StubInterface>
  auto createClientWithCredentials(
      const grpc::string &address,
      std::shared_ptr<grpc::ChannelCredentials> chan_creds) {
    return StubInterface::NewStub(grpc::CreateCustomChannel(
        address, chan_creds, details::getChannelArguments<StubInterface>()));
  }

  /**
   * Creates gRPC client without TLS use
   * @tparam StubInterface - gRPC service type
   * @param address where to connect including port number
   * @return gRPC stub
   */
  template <typename StubInterface>
  auto createInsecureClient(const grpc::string &address) {
    return createClientWithCredentials<StubInterface>(
        address, grpc::InsecureChannelCredentials());
  }

  /**
   * Creates gRPC client secured via TLS
   * @tparam StubInterface - gRPC service type
   * @param address where to connect
   * @param root_certificates_pem - can be an empty string. Otherwise a string
   * with a list of PEM encoded root certificates of the server
   * @return gRPC stub
   */
  template <typename StubInterface>
  auto createSecureClient(const grpc::string &address,
                          std::string root_certificates_pem) {
    auto ssl_options = grpc::SslCredentialsOptions();
    ssl_options.pem_root_certs = root_certificates_pem;
    auto credentials = grpc::SslCredentials(ssl_options);
    return createClientWithCredentials<StubInterface>(address, credentials);
  }
}  // namespace fc::network

#endif  // CPP_FILECOIN_CORE_NETWORK_GRPC_CHANNEL_BUILDER_HPP
