/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <memory>
#include <string>

#include "drand/impl/client.hpp"

/*
 * macOS may require setting the env var:
 * GRPC_DEFAULT_SSL_ROOTS_FILE_PATH=/etc/ssl/cert.pem
 *
 * This solves gRPC issue of looking the OS's certificates in the wrong place.
 */

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Please pass hostname and port separated with space"
              << std::endl;
    return 0;
  }

  std::shared_ptr<fc::drand::DrandSyncClient> client =
      std::make_shared<fc::drand::DrandSyncClientImpl>(
          std::string(argv[1]), std::stoi(std::string(argv[2])));

  if (auto result = client->publicRand(1)) {
    std::cout << static_cast<int>(result.has_value()) << std::endl;
    for (auto e : result.value().randomness) {
      std::cout << e;
    }
    std::cout << std::endl;
  }

  if (auto result = client->group()) {
    std::cout << "Dist Keys " << result.value().dist_key.size() << std::endl;
  }
  return 0;
}
