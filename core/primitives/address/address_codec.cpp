/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "address_codec.hpp"

#include <cppcodec/base32_default_rfc4648.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <stdexcept>

#include "common/visitor.hpp"
#include "crypto/blake2/blake2b.h"
#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls/bls_types.hpp"

namespace fc::primitives::address {

  static const size_t kBlsPublicKeySize{
      std::tuple_size<fc::crypto::bls::PublicKey>::value};

  using common::Blob;
  using base32 = cppcodec::base32_rfc4648;
  using UVarint = libp2p::multi::UVarint;

  std::vector<uint8_t> encode(const Address &address) noexcept {
    std::vector<uint8_t> res{};
    res.push_back(address.getProtocol());
    std::vector<uint8_t> payload = visit_in_place(
        address.data,
        [](uint64_t v) { return UVarint{v}.toVector(); },
        [](const auto &v) { return std::vector<uint8_t>(v.begin(), v.end()); });
    res.insert(res.end(),
               std::make_move_iterator(payload.begin()),
               std::make_move_iterator(payload.end()));
    return res;
  }

  outcome::result<Address> decode(const std::vector<uint8_t> &v) {
    if (v.size() < 2) return outcome::failure(AddressError::INVALID_PAYLOAD);

    // TODO(ekovalev): [FIL-118] make network configurable; hardcoded for now
    Network net{Network::TESTNET};

    auto p = static_cast<Protocol>(v[0]);
    std::vector<uint8_t> payload(std::next(v.begin()), v.end());
    switch (p) {
      case Protocol::ID: {
        boost::optional<UVarint> value = UVarint::create(payload);
        if (value) {
          return Address{net, value->toUInt64()};
        }
        return outcome::failure(AddressError::INVALID_PAYLOAD);
      }
      case Protocol::SECP256K1: {
        if (payload.size() != fc::crypto::blake2b::BLAKE2B160_HASH_LENGTH) {
          return outcome::failure(AddressError::INVALID_PAYLOAD);
        }
        Secp256k1PublicKeyHash hash{};
        std::copy_n(std::make_move_iterator(payload.begin()),
                    fc::crypto::blake2b::BLAKE2B160_HASH_LENGTH,
                    hash.begin());
        return Address{net, hash};
      }
      case Protocol::ACTOR: {
        if (payload.size() != fc::crypto::blake2b::BLAKE2B160_HASH_LENGTH) {
          return outcome::failure(AddressError::INVALID_PAYLOAD);
        }
        ActorExecHash hash{};
        std::copy_n(std::make_move_iterator(payload.begin()),
                    fc::crypto::blake2b::BLAKE2B160_HASH_LENGTH,
                    hash.begin());
        return Address{net, hash};
      }
      case Protocol::BLS: {
        if (payload.size() != kBlsPublicKeySize) {
          return outcome::failure(AddressError::INVALID_PAYLOAD);
        }
        BLSPublicKeyHash hash{};
        std::copy_n(std::make_move_iterator(payload.begin()),
                    kBlsPublicKeySize,
                    hash.begin());
        return Address{net, hash};
      }
      default:
        return outcome::failure(AddressError::UNKNOWN_PROTOCOL);
    }
  }

  std::string encodeToString(const Address &address) {
    std::string res{};

    char networkPrefix = address.network == Network::TESTNET ? 't' : 'f';
    res.push_back(networkPrefix);

    Protocol p = address.getProtocol();
    res.append(std::to_string(p));

    if (p == Protocol::ID) {
      const uint64_t *data_ptr = boost::get<uint64_t>(&address.data);
      if (data_ptr != nullptr) res.append(std::to_string(*data_ptr));
      return res;
    }

    std::vector<uint8_t> payload = visit_in_place(
        address.data,
        [](uint64_t v) { return UVarint{v}.toVector(); },
        [](const auto &v) { return std::vector<uint8_t>(v.begin(), v.end()); });

    std::vector chksum = checksum(address);

    payload.insert(payload.end(),
                   std::make_move_iterator(chksum.begin()),
                   std::make_move_iterator(chksum.end()));
    auto encoded = base32::encode(payload);

    // Convert to lower case, as the spec requires:
    // https://filecoin-project.github.io/specs/#payload
    std::transform(
        encoded.begin(), encoded.end(), encoded.begin(), [](unsigned char c) {
          return std::tolower(c);
        });
    res.append(encoded);
    // Remove padding '=' symbols
    res.erase(
        std::remove_if(
            res.begin(), res.end(), [](unsigned char x) { return x == '='; }),
        res.end());
    return res;
  }

  outcome::result<Address> decodeFromString(const std::string &s) {
    if (s.size() < 3) return outcome::failure(AddressError::INVALID_PAYLOAD);

    if (s[0] != 'f' && s[0] != 't')
      return outcome::failure(AddressError::UNKNOWN_NETWORK);

    std::vector<uint8_t> buffer{};
    Network net = s[0] == 't' ? Network::TESTNET : Network::MAINNET;

    int protocol = int(s[1]) - int('0');
    if (protocol < Protocol::ID || protocol > Protocol::BLS)
      return outcome::failure(AddressError::UNKNOWN_PROTOCOL);

    std::string tail = s.substr(2);
    if (protocol == Protocol::ID) {
      try {
        auto value = static_cast<uint64_t>(std::stoul(tail));
        return Address{net, value};

      } catch (std::invalid_argument &e) {
        return outcome::failure(AddressError::INVALID_PAYLOAD);

      } catch (std::out_of_range &e) {
        return outcome::failure(AddressError::INVALID_PAYLOAD);
      }
    }

    buffer.push_back(static_cast<uint8_t>(protocol));

    // transform the base32-encoded part of the input to upper case
    std::transform(tail.begin(), tail.end(), tail.begin(), [](char c) {
      return std::toupper(c);
    });

    // Pad spayload with '=' on the right to make its length a multiple of 8
    auto numPads = 8l - static_cast<int64_t>(tail.size() % 8);
    if (numPads < 8)
      for (int i = 0; i < numPads; i++) tail.push_back('=');

    auto payload = base32::decode(tail);

    if (payload.size() < 4)
      return outcome::failure(AddressError::INVALID_PAYLOAD);

    // Copy the decoded payload except last 4 bytes of the checksum to the
    // buffer
    buffer.insert(buffer.end(),
                  std::make_move_iterator(payload.begin()),
                  std::make_move_iterator(payload.end()) - 4);

    OUTCOME_TRY(address, decode(buffer));
    address.network = net;
    return std::move(address);
  }

  std::vector<uint8_t> checksum(const Address &address) {
    std::vector<uint8_t> res{};

    Protocol p = address.getProtocol();
    if (p == Protocol::ID) {
      // Checksum is not defined for an ID Address
      return res;
    }

    res.resize(4);

    std::vector<uint8_t> ingest{};

    ingest.push_back(p);

    ingest = visit_in_place(address.data,
                            [&ingest](uint64_t v) { return ingest; },
                            [&ingest](const auto &v) {
                              ingest.insert(ingest.end(), v.begin(), v.end());
                              return ingest;
                            });

    size_t outlen = 4;
    blake2b(res.data(), outlen, nullptr, 0, ingest.data(), ingest.size());
    return res;
  }

  bool validateChecksum(const Address &address,
                        const std::vector<uint8_t> &expect) {
    std::vector<uint8_t> digest = checksum(address);
    return digest == expect;
  }

};  // namespace fc::primitives::address
