/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filesystem_keystore.hpp"

#include "primitives/address/address_codec.hpp"
#include "storage/filestore/filestore_error.hpp"
#include "storage/filestore/impl/filesystem/filesystem_filestore.hpp"

using fc::primitives::address::Address;
using fc::primitives::address::Protocol;
using fc::storage::filestore::FileStoreError;
using fc::storage::filestore::FileSystemFileStore;
using fc::storage::filestore::Path;
using fc::storage::keystore::FileSystemKeyStore;
using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;

FileSystemKeyStore::FileSystemKeyStore(
    Path path,
    std::shared_ptr<BlsProvider> blsProvider,
    std::shared_ptr<Secp256k1Provider> secp256K1Provider)
    : KeyStore(std::move(blsProvider), std::move(secp256K1Provider)),
      keystore_path_(std::move(path)),
      filestore_(std::make_shared<FileSystemFileStore>()) {}

fc::outcome::result<bool> FileSystemKeyStore::has(const Address &address) const
    noexcept {
  OUTCOME_TRY(path, addressToPath(address));
  OUTCOME_TRY(exists, filestore_->exists(path));
  return exists;
}

fc::outcome::result<void> FileSystemKeyStore::put(
    Address address, typename KeyStore::TPrivateKey key) noexcept {
  OUTCOME_TRY(valid, checkAddress(address, key));
  if (!valid) return KeyStoreError::WRONG_ADDRESS;
  OUTCOME_TRY(exists, has(address));
  if (exists) return KeyStoreError::ALREADY_EXISTS;
  OUTCOME_TRY(path, addressToPath(address));
  OUTCOME_TRY(file, filestore_->create(path));

  if (address.getProtocol() == Protocol::BLS) {
    auto bls_private_key = boost::get<BlsPrivateKey>(key);
    OUTCOME_TRY(write_size, file->write(0, bls_private_key));
    if (write_size != bls_private_key.size()) {
      return KeyStoreError::CANNOT_STORE;
    }
    return fc::outcome::success();
  }
  if (address.getProtocol() == Protocol::SECP256K1) {
    auto secp256k1_private_key = boost::get<Secp256k1PrivateKey>(key);
    OUTCOME_TRY(write_size, file->write(0, secp256k1_private_key));
    if (write_size != secp256k1_private_key.size()) {
      return KeyStoreError::CANNOT_STORE;
    }
    return fc::outcome::success();
  }

  return KeyStoreError::WRONG_ADDRESS;
}

fc::outcome::result<void> FileSystemKeyStore::remove(
    const Address &address) noexcept {
  OUTCOME_TRY(found, has(address));
  if (!found) return KeyStoreError::NOT_FOUND;
  OUTCOME_TRY(path, addressToPath(address));
  OUTCOME_TRY(filestore_->remove(path));
  return fc::outcome::success();
}

fc::outcome::result<std::vector<Address>> FileSystemKeyStore::list() const
    noexcept {
  OUTCOME_TRY(files, filestore_->list(keystore_path_));
  std::vector<Address> res;
  res.reserve(files.size());

  for (const auto &file : files) {
    std::size_t from = file.find_last_of(filestore::DELIMITER) + 1;
    std::size_t to = file.rfind(this->kPrivateKeyExtension);
    OUTCOME_TRY(address,
                fc::primitives::address::decodeFromString(
                    file.substr(from, to - from)));
    res.emplace_back(std::move(address));
  }
  return std::move(res);
}

fc::outcome::result<typename KeyStore::TPrivateKey> FileSystemKeyStore::get(
    const Address &address) const noexcept {
  OUTCOME_TRY(found, has(address));
  if (!found) return KeyStoreError::NOT_FOUND;
  OUTCOME_TRY(path, addressToPath(address));
  OUTCOME_TRY(file, filestore_->open(path));

  if (address.getProtocol() == Protocol::BLS) {
    BlsPrivateKey private_key{};
    OUTCOME_TRY(read_size, file->read(0, private_key));
    if (read_size != private_key.size()) {
      return KeyStoreError::CANNOT_READ;
    }

    return private_key;
  }
  if (address.getProtocol() == Protocol::SECP256K1) {
    Secp256k1PrivateKey private_key{};
    OUTCOME_TRY(read_size, file->read(0, private_key));
    if (read_size != private_key.size()) {
      return KeyStoreError::CANNOT_READ;
    }

    return private_key;
  }

  return KeyStoreError::WRONG_ADDRESS;
}

fc::outcome::result<Path> FileSystemKeyStore::addressToPath(
    const Address &address) const noexcept {
  std::stringstream ss;
  ss << fc::primitives::address::encodeToString(address);
  Path res =
      keystore_path_ + filestore::DELIMITER + ss.str() + kPrivateKeyExtension;

  return res;
}
