/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/crypto/sha/sha256.hpp>

#include "common/hexutil.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "drand/example/example_constants.hpp"
#include "drand/messages.hpp"

namespace k = fc::drand::example;

void print(std::string title, gsl::span<const uint8_t> bytes) {
  std::cout << title << ":" << std::endl;
  std::cout << fc::common::hex_lower(bytes) << std::endl;
  std::cout << std::string(80, '-') << std::endl;
}

#define PRINT(var) print(#var, (var));

std::vector<uint8_t> load(std::string filename) {
  std::vector<uint8_t> result;
  std::ifstream in(filename, std::ifstream::binary);
  in.unsetf(std::ios::skipws);
  in.seekg(0, std::ios::end);
  result.reserve(in.tellg());
  in.seekg(0, std::ios::beg);
  result.insert(result.begin(),
                std::istream_iterator<uint8_t>(in),
                std::istream_iterator<uint8_t>());
  return result;
}

fc::drand::PublicRandResponse loadBeacon(uint64_t round) {
  auto number = std::to_string(round);
  return fc::drand::PublicRandResponse{
      .round = round,
      .signature = load(std::string(k::kSigFile) + number),
      .previous_signature = load(std::string(k::kPreviousSigFile) + number),
      .randomness = load(std::string(k::kRandomnessFile) + number)};
};

/// number says how many coefs to load
std::vector<fc::drand::Bytes> loadCoefs(size_t number) {
  std::vector<fc::drand::Bytes> result;
  for (size_t i = 0; i < number; ++i) {
    auto coef = load(std::string(k::kGroupCoefFile) + std::to_string(i));
    result.emplace_back(std::move(coef));
  }
  return result;
}

auto message(uint64_t current_round, const std::vector<uint8_t> &previous_sig) {
  auto buffer = previous_sig;
  libp2p::common::putUint64BE(buffer, current_round);
  print("message before sha256", buffer);
  auto hash = libp2p::crypto::sha256(buffer);
  return hash;
}

/**
 * Reads from the working directory three drand rounds data
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
  auto second = loadBeacon(2);
  auto third = loadBeacon(3);
  assert(third.previous_signature == second.signature);
  PRINT(third.signature);
  PRINT(third.previous_signature);

  auto msg = message(3, third.previous_signature);
  PRINT(msg);

  auto coefs = loadCoefs(4);
  auto &&key = coefs[0];
  PRINT(key);

  auto bls = std::make_unique<fc::crypto::bls::BlsProviderImpl>();

  fc::crypto::bls::Signature signature;
  fc::crypto::bls::PublicKey public_key;
  std::copy(third.signature.begin(), third.signature.end(), signature.begin());
  std::copy(key.begin(), key.end(), public_key.begin());

  PRINT(signature);
  PRINT(public_key);
  auto result = bls->verifySignature(msg, signature, public_key);

  if (result) {
    std::cout << (result.value() ? "success" : "failure") << std::endl;
  } else {
    std::cout << "noo" << std::endl;
  }

  return 0;
}
