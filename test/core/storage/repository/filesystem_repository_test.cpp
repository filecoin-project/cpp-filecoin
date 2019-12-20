/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/impl/filesystem_repository.hpp"

#include <storage/repository/repository_error.hpp>

#include "crypto/bls_provider/impl/bls_provider_impl.hpp"
#include "primitives/address/impl/address_builder_impl.hpp"
#include "storage/ipfs/datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::common::Buffer;
using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::address::AddressBuilderImpl;
using fc::storage::repository::FileSystemRepository;
using fc::storage::repository::Repository;
using fc::storage::repository::RepositoryError;
using BlsKeyPair = fc::crypto::bls::KeyPair;
using fc::primitives::Network;
using libp2p::multi::ContentIdentifier;
using libp2p::multi::HashType;
using libp2p::multi::MulticodecType;
using libp2p::multi::Multihash;

class FilesSystemRepositoryTest : public test::BaseFS_Test {
 public:
  FilesSystemRepositoryTest()
      : test::BaseFS_Test("/tmp/fc_filesystem_repository_test/") {}

  std::string api_address{"api_address string"};
  leveldb::Options leveldb_options{};

  Repository::Version version_expected = 1;

  /**
   * Create a test repository with an empty file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();

    leveldb_options.create_if_missing = true;
  }
};

/**
 * @given Empty directory
 * @when Repository is created in the directory
 * @then Files and folders are created:
 * - api with api path
 * - keys directory
 * - datastore
 * - version with version 1
 * - repo.lock
 */
TEST_F(FilesSystemRepositoryTest, EmptyDirectory) {
  EXPECT_OUTCOME_TRUE_1(FileSystemRepository::create(
      base_path.string(), api_address, leveldb_options));

  ASSERT_TRUE(exists(FileSystemRepository::kApiFilename));
  boost::filesystem::ifstream api_is(base_path
                                     / FileSystemRepository::kApiFilename);
  std::string api_actual;
  std::getline(api_is, api_actual);
  ASSERT_EQ(api_actual, api_address);

  ASSERT_TRUE(exists(FileSystemRepository::kKeysDirectory));
  ASSERT_TRUE(exists(FileSystemRepository::kDatastore));

  ASSERT_TRUE(exists(FileSystemRepository::kVersionFilename));
  boost::filesystem::ifstream version_is(
      base_path / FileSystemRepository::kVersionFilename);
  Repository::Version version_actual;
  version_is >> version_actual;
  ASSERT_EQ(version_actual, version_expected);

  ASSERT_TRUE(exists(FileSystemRepository::kRepositoryLock));
}

/**
 * @given Directory with repository
 * @when Try open directory and read saved data
 * @then Data is read
 */
TEST_F(FilesSystemRepositoryTest, PersistenceRepository) {
  auto config_path = fs::canonical(createFile("config.json")).string();
  std::ofstream config_file(config_path);
  config_file << "{\n"
                 "  \"param\": \"value\"\n"
                 "}";
  config_file.close();

  EXPECT_OUTCOME_TRUE(repository_old,
                      FileSystemRepository::create(
                          base_path.string(), api_address, leveldb_options));

  // set key
  auto keystore = repository_old->getKeyStore();
  AddressBuilderImpl addressBuilder{};
  auto bls_provider = std::make_shared<BlsProviderImpl>();
  auto bls_keypair =
      std::make_shared<BlsKeyPair>(bls_provider->generateKeyPair().value());
  auto bls_address =
      addressBuilder
          .makeFromBlsPublicKey(Network::MAINNET, bls_keypair->public_key)
          .value();
  EXPECT_OUTCOME_TRUE_1(keystore->put(bls_address, bls_keypair->private_key));
  keystore.reset();

  // save ipld data
  auto datastore = repository_old->getIpldStore();

  ContentIdentifier cid{
      ContentIdentifier::Version::V1,
      MulticodecType::SHA2_256,
      Multihash::create(HashType::sha256,
                        "0123456789ABCDEF0123456789ABCDEF"_unhex)
          .value()};
  Buffer value{"0123456789ABCDEF0123456789ABCDEF"_unhex};
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  datastore.reset();

  // delete repository
  repository_old.reset();

  // open again
  EXPECT_OUTCOME_TRUE(repository,
                      FileSystemRepository::create(
                          base_path.string(), api_address, leveldb_options));

  auto config = repository->getConfig();
  EXPECT_OUTCOME_EQ(config->get<std::string>("param"), "value");

  keystore = repository->getKeyStore();
  EXPECT_OUTCOME_EQ(keystore->has(bls_address), true);

  datastore = repository->getIpldStore();
  EXPECT_OUTCOME_EQ(datastore->contains(cid), true);
}

/**
 * @given Repository with wrong version number
 * @when Try open repository
 * @then Error WRONG_VERSION returned
 */
TEST_F(FilesSystemRepositoryTest, WrongVersion) {
  auto version_path = fs::canonical(createFile("version")).string();
  std::ofstream version_file(version_path);
  version_file << "123" << std::endl;
  version_file.close();

  EXPECT_OUTCOME_ERROR(RepositoryError::WRONG_VERSION,
                       FileSystemRepository::create(
                           base_path.string(), api_address, leveldb_options));
}

/**
 * @given Repository with invalid version number
 * @when Try open repository
 * @then Error WRONG_VERSION returned
 */
TEST_F(FilesSystemRepositoryTest, InvalidVersion) {
  auto version_path = fs::canonical(createFile("version")).string();
  std::ofstream version_file(version_path);
  version_file << "invalid version number" << std::endl;
  version_file.close();

  EXPECT_OUTCOME_ERROR(RepositoryError::WRONG_VERSION,
                       FileSystemRepository::create(
                           base_path.string(), api_address, leveldb_options));
}
