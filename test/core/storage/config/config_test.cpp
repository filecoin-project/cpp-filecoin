/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/config/config.hpp"

#include <gtest/gtest.h>

#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::config::ConfigError;

class ConfigImplTest : public test::BaseFS_Test {
 public:
  ConfigImplTest() : test::BaseFS_Test("fc_config_test") {}

  /**
   * Create a test directory with an empty file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
  }

  fc::storage::config::Config config{};
};

/**
 * @given path to a file not exists
 * @when try to load file not exists
 * @then ConfigError::CANNOT_OPEN_FILE returned
 */
TEST_F(ConfigImplTest, FileNotFound) {
  std::string path_not_exists = "/not/exists/file/config";
  EXPECT_OUTCOME_ERROR(ConfigError::CANNOT_OPEN_FILE,
                       config.load(path_not_exists));
}

/**
 * @given an empty file
 * @when open the file
 * @then error returned
 */
TEST_F(ConfigImplTest, EmptyFile) {
  auto empty_file_path = fs::canonical(createFile("empty.txt")).string();
  EXPECT_OUTCOME_ERROR(ConfigError::JSON_PARSER_ERROR,
                       config.load(empty_file_path));
}

/**
 * @given a file with JSON-invalid content
 * @when open the file
 * @then error returned
 */
TEST_F(ConfigImplTest, InvalidFile) {
  auto file_path = fs::canonical(createFile("wrong_json.txt")).string();
  std::ofstream file(file_path);
  file << "not a valid JSON content";
  file.flush();
  EXPECT_OUTCOME_ERROR(ConfigError::JSON_PARSER_ERROR, config.load(file_path));
}

/**
 * @given loaded JSON config with value
 * @when try read key that doesn't exist
 * @then BAD_PATH error returned
 */
TEST_F(ConfigImplTest, ReadKeyNotExists) {
  auto file_path = fs::canonical(createFile("config.json")).string();
  std::ofstream file1(file_path);
  file1 << "{\n"
           "  \"config\": {\n"
           "    \"field\": \"1\"\n"
           "  }\n"
           "}";
  file1.flush();
  EXPECT_OUTCOME_TRUE_1(config.load(file_path));
  EXPECT_OUTCOME_TRUE(val, config.get<int>("config.field"));
  EXPECT_EQ(1, val);
  EXPECT_OUTCOME_ERROR(ConfigError::BAD_PATH, config.get<int>("not.exists"));
}

/**
 * @given loaded JSON config with values
 * @when try read values of different types
 * @then values returned
 */
TEST_F(ConfigImplTest, ReadTypes) {
  auto file_path = fs::canonical(createFile("config.json")).string();
  std::ofstream file1(file_path);
  file1 << "{\n"
           "  \"config\": {\n"
           "    \"int\": \"1\",\n"
           "    \"str\": \"text\",\n"
           "    \"bool\": \"true\",\n"
           "    \"double\": \"1.23\"\n"
           "  }\n"
           "}";
  file1.flush();
  EXPECT_OUTCOME_TRUE_1(config.load(file_path));
  EXPECT_OUTCOME_TRUE(int_val, config.get<int>("config.int"));
  EXPECT_EQ(1, int_val);
  EXPECT_OUTCOME_TRUE(str_val, config.get<std::string>("config.str"));
  EXPECT_EQ("text", str_val);
  EXPECT_OUTCOME_TRUE(bool_val, config.get<bool>("config.bool"));
  EXPECT_EQ(true, bool_val);
  EXPECT_OUTCOME_TRUE(double_val, config.get<double>("config.double"));
  EXPECT_DOUBLE_EQ(1.23, double_val);
}

/**
 * @given a file with JSON content
 * @when open the file and get property
 * @then value returned
 */
TEST_F(ConfigImplTest, ReadJson) {
  auto file_path = fs::canonical(createFile("config.json")).string();
  std::ofstream file(file_path);
  file << "{\n"
          "  \"employee\": {\n"
          "    \"name\": \"John Smith\",\n"
          "    \"projects\": { \"project\": \"filecoin\" },\n"
          "    \"age\": \"33\"\n"
          "  }\n"
          "}";
  file.flush();
  EXPECT_OUTCOME_TRUE_1(config.load(file_path));

  EXPECT_OUTCOME_TRUE(name, config.get<std::string>("employee.name"));
  EXPECT_EQ("John Smith", name);
  EXPECT_OUTCOME_TRUE(project,
                      config.get<std::string>("employee.projects.project"));
  EXPECT_EQ("filecoin", project);
  EXPECT_OUTCOME_TRUE(age, config.get<int>("employee.age"));
  EXPECT_EQ(33, age);
}

/**
 * @given 2 files with JSON content
 * @when load file and then load again
 * @then first config is unaccessible, 2nd is accessible
 */
TEST_F(ConfigImplTest, ReadTwiceJson) {
  auto file_path1 = fs::canonical(createFile("config1.json")).string();
  std::ofstream file1(file_path1);
  file1 << "{\n"
           "  \"config\": {\n"
           "    \"field1\": \"1\"\n"
           "  }\n"
           "}";
  file1.flush();
  EXPECT_OUTCOME_TRUE_1(config.load(file_path1));

  EXPECT_OUTCOME_TRUE(val1, config.get<int>("config.field1"));
  EXPECT_EQ(1, val1);

  auto file_path2 = fs::canonical(createFile("config2.json")).string();
  std::ofstream file2(file_path2);
  file2 << "{\n"
           "  \"config\": {\n"
           "    \"field2\": \"2\"\n"
           "  }\n"
           "}";
  file2.flush();
  EXPECT_OUTCOME_TRUE_1(config.load(file_path2));

  EXPECT_OUTCOME_ERROR(ConfigError::BAD_PATH, config.get<int>("config.field1"));
  EXPECT_OUTCOME_TRUE(val2, config.get<int>("config.field2"));
  EXPECT_EQ(2, val2);
}

/**
 * @given a config and a correct file path
 * @when save is called with correct path
 * @then config is saved
 */
TEST_F(ConfigImplTest, SaveConfig) {
  auto filename = getPathString() + "/config.json";

  EXPECT_OUTCOME_TRUE_1(config.set("config.int", 1));
  EXPECT_OUTCOME_TRUE_1(config.set("config.str", "text"));
  EXPECT_OUTCOME_TRUE_1(config.set("config.bool", true));
  EXPECT_OUTCOME_TRUE_1(config.set("config.double", 1.23));
  EXPECT_OUTCOME_TRUE_1(config.save(filename));

  EXPECT_OUTCOME_TRUE_1(config.load(filename));
  EXPECT_OUTCOME_TRUE(int_val, config.get<int>("config.int"));
  EXPECT_EQ(1, int_val);
  EXPECT_OUTCOME_TRUE(str_val, config.get<std::string>("config.str"));
  EXPECT_EQ("text", str_val);
  EXPECT_OUTCOME_TRUE(bool_val, config.get<bool>("config.bool"));
  EXPECT_EQ(true, bool_val);
  EXPECT_OUTCOME_TRUE(double_val, config.get<double>("config.double"));
  EXPECT_DOUBLE_EQ(1.23, double_val);
}

/**
 * @given a config and a invalid file path
 * @when save is called with invalid path
 * @then CANNOT_OPEN_FILE error returned
 */
TEST_F(ConfigImplTest, SaveInvalidPath) {
  auto filename = "[:\\\\\\\\/*\\\"?|<>']";
  EXPECT_OUTCOME_ERROR(ConfigError::CANNOT_OPEN_FILE, config.save(filename));
}
