/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "drand/example/example_constants.hpp"
#include "drand/impl/client.hpp"

namespace k = fc::drand::example;

/*
 * macOS may require setting the env var:
 * GRPC_DEFAULT_SSL_ROOTS_FILE_PATH=/etc/ssl/cert.pem
 *
 * This solves gRPC issue of looking the OS's certificates in the wrong place.
 */

void dump(std::string filename, const std::vector<uint8_t> &data) {
  std::ofstream out(filename, std::ofstream::binary);
  out.write(reinterpret_cast<const char *>(data.data()), data.size());
  out.flush();
  out.close();
}

/**
 * Acquires drand beacons for the first three rounds and saves signatures to the
 * files
 * @param argc
 * @param argv hostname and port of drand node to read from
 * @return
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

  std::vector<uint8_t> previous_signature;
  for (auto i = 1u; i < k::kRounds + 1; ++i) {
    auto result = client->publicRand(i);
    if (not result) {
      std::cout << "Error retrieving randomness for the round " << i
                << std::endl;
      return 1;
    }

    auto &&response = result.value();

    if (1 != i) {
      if (response.previous_signature != previous_signature) {
        std::cout << "Previous signature mismatch in response for the " << i
                  << " round" << std::endl;
      }
    }
    previous_signature = response.signature;

    auto number = std::to_string(i);
    dump(std::string(k::kSigFile) + number, response.signature);
    dump(std::string(k::kPreviousSigFile) + number,
         response.previous_signature);
    dump(std::string(k::kRandomnessFile) + number, response.randomness);
  }

  auto group = client->group();
  if (not group) {
    std::cout << "Cannot acquire nodes group info" << std::endl;
    return 1;
  }

  auto &&group_resp = group.value();
  for (auto i = 0u; i < group_resp.dist_key.size(); ++i) {
    dump(std::string(k::kGroupCoefFile) + std::to_string(i),
         group_resp.dist_key[i]);
  }

  std::cout << "Done" << std::endl;
  return 0;
}
