/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <memory>

#include "drand/impl/client.hpp"

/*
 * macOS may require setting the env var:
 * GRPC_DEFAULT_SSL_ROOTS_FILE_PATH=/etc/ssl/cert.pem
 *
 * This solves gRPC issue of looking the OS's certificates in the wrong place.
 */

int main() {
  std::shared_ptr<fc::drand::DrandSyncClient> client =
      std::make_shared<fc::drand::DrandSyncClientImpl>(
          "s5.stg2.fuhon.soramitsu.co.jp", 8081);

  auto result = client->publicRand(1);
  std::cout << (int)result.has_value() << std::endl;
  for (auto e : result.value().randomness) {
    std::cout << e;
  }
  return 0;
}
