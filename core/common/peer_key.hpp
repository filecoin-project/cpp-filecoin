/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/string_file.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/key.hpp>

#include "common/blob.hpp"
#include "common/outcome.hpp"
#include "common/span.hpp"

namespace fc {
  // TODO: use PEM or json format
  inline outcome::result<libp2p::crypto::KeyPair> loadPeerKey(
      const boost::filesystem::path &path) {
    libp2p::crypto::ed25519::Ed25519ProviderImpl provider;
    libp2p::crypto::ed25519::Keypair ed;
    if (false) {
      std::string str;
      boost::filesystem::load_string_file(path, str);
      OUTCOME_TRYA(ed.private_key,
                   common::Blob<32>::fromSpan(common::span::cbytes(str)));
      OUTCOME_TRYA(ed.public_key, provider.derive(ed.private_key));
    } else {
      OUTCOME_TRYA(ed, provider.generate());
      boost::filesystem::save_string_file(
          path, std::string{common::span::bytestr(ed.private_key)});
    }
    libp2p::crypto::KeyPair keys;
    keys.privateKey.type = keys.publicKey.type =
        libp2p::crypto::Key::Type::Ed25519;
    keys.privateKey.data.assign(ed.private_key.begin(), ed.private_key.end());
    keys.publicKey.data.assign(ed.public_key.begin(), ed.public_key.end());
    return keys;
  }
}  // namespace fc
