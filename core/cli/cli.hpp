/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include <boost/program_options/options_description.hpp>
#include <map>
#include <memory>
#include <typeindex>

#include "cli/try.hpp"

#define CLI_BOOL(NAME, DESCRIPTION)                               \
  struct {                                                        \
    bool v{};                                                     \
    void operator()(Opts &opts) {                                 \
      opts.add_options()(NAME, po::bool_switch(&v), DESCRIPTION); \
    }                                                             \
    operator bool() const {                                       \
      return v;                                                   \
    }                                                             \
  }

#define CLI_DEFAULT(NAME, DESCRIPTION, TYPE, INIT)          \
  struct {                                                  \
    TYPE v INIT;                                            \
    void operator()(Opts &opts) {                           \
      opts.add_options()(NAME, po::value(&v), DESCRIPTION); \
    }                                                       \
    auto &operator*() const {                               \
      return v;                                             \
    }                                                       \
    auto &operator*() {                                     \
      return v;                                             \
    }                                                       \
    auto *operator->() const {                              \
      return &v;                                            \
    }                                                       \
    auto *operator->() {                                    \
      return &v;                                            \
    }                                                       \
  }

#define CLI_OPTIONAL(NAME, DESCRIPTION, TYPE)                              \
  struct {                                                                 \
    boost::optional<TYPE> v;                                               \
    void operator()(Opts &opts) {                                          \
      opts.add_options()(NAME, po::value(&v), DESCRIPTION);                \
    }                                                                      \
    operator bool() const {                                                \
      return v.operator bool();                                            \
    }                                                                      \
    void check() const {                                                   \
      if (!v) {                                                            \
        throw ::fc::cli::CliError{"--{} argument is required but missing", \
                                  NAME};                                   \
      }                                                                    \
    }                                                                      \
    auto &operator*() const {                                              \
      check();                                                             \
      return *v;                                                           \
    }                                                                      \
    auto &operator*() {                                                    \
      check();                                                             \
      return *v;                                                           \
    }                                                                      \
    auto *operator->() const {                                             \
      return &**this;                                                      \
    }                                                                      \
    auto *operator->() {                                                   \
      return &**this;                                                      \
    }                                                                      \
  }

#define CLI_OPTS() ::fc::cli::Opts opts()
#define CLI_RUN()                  \
  static ::fc::cli::RunResult run( \
      ::fc::cli::ArgsMap &argm, Args &args, ::fc::cli::Argv &&argv)
#define CLI_NO_RUN() constexpr static std::nullptr_t run{nullptr};

namespace fc::cli {
  namespace po = boost::program_options;
  using Opts = po::options_description;

  using RunResult = void;
  struct ArgsMap {
    std::map<std::type_index, std::shared_ptr<void>> _;
    template <typename Args>
    void add(Args &&v) {
      _.emplace(typeid(Args), std::make_shared<Args>(std::forward<Args>(v)));
    }
    template <typename Cmd>
    typename Cmd::Args &of() {
      return *reinterpret_cast<typename Cmd::Args *>(
          _.at(typeid(typename Cmd::Args)).get());
    }
  };
  // note: Args is defined inside command
  using Argv = std::vector<std::string>;

  const std::string &cliArgv(const Argv &argv,
                             size_t i,
                             const std::string_view &name) {
    if (i < argv.size()) {
      return argv[i];
    }
    throw CliError{"positional argument {} is required but missing", name};
  }
  template <typename T>
  T cliArgv(const std::string &arg, const std::string_view &name) {
    boost::any out;
    try {
      po::value<T>()->xparse(out, Argv{arg});
    } catch (po::validation_error &e) {
      e.set_option_name(std::string{name});
      throw;
    }
    return boost::any_cast<T>(out);
  }
  template <typename T>
  T cliArgv(const Argv &argv, size_t i, const std::string_view &name) {
    return cliArgv<T>(cliArgv(argv, i, name), name);
  }

  struct Empty {
    struct Args {
      CLI_OPTS() {
        return {};
      }
    };
    CLI_NO_RUN();
  };
  using Group = Empty;

  struct ShowHelp {};

  class TableWriter {
    struct Column {
      std::string name;
      bool separate_line;
      int lines = 0;
    };
    using Row = std::map<int, std::string>;

    std::vector<Column> columns;
    std::vector<Row> rows;

   public:
    static Column newColumn(const std::string &name) {
      Column column{.name = name, .separate_line = false};
      return column;
    }

    static Column newLineColumn(const std::string &name) {
      Column column{.name = name, .separate_line = true};
      return column;
    }

    explicit TableWriter(const std::vector<Column> &cols) {
      columns = cols;
    }

    void write(std::map<std::string, std::string> range) {
      std::map<int, std::string> byColumnId;
      bool flag = false;

      while (not flag) {
        for (auto &[columnName, value] : range) {
          for (size_t i = 0; i < columns.size(); ++i) {
            Column &column = columns[i];
            if (column.name == columnName) {
              byColumnId[i] = std::move(value);
              range.erase(columnName);
              column.lines++;
              flag = true;
              break;
            }
          }

          if (flag) {
            break;
          }
          byColumnId[columns.size()] = std::move(value);
          range.erase(columnName);
          columns.push_back(
              Column{.name = columnName, .separate_line = false, .lines = 1});
        }
        flag = not flag;
      }

      rows.push_back(byColumnId);
    }

    void flush() {
      std::vector<int> colLenghts(columns.size());
      // colLenghts.reserve(columns.size());

      std::map<int, std::string> header;
      for (size_t i = 0; i < columns.size(); ++i) {
        Column &column = columns[i];
        if (column.separate_line) {
          continue;
        }
        header[i] = column.name;
      }

      rows.insert(rows.begin(), header);

      for (size_t i = 0; i < columns.size(); ++i) {
        Column &column = columns[i];
        if (column.lines == 0) {
          continue;
        }

        for (size_t j = 0; j < rows.size(); ++j) {
          Row &row = rows[j];
          if (row.find(i) == row.end()) {
            continue;
          }
          std::string value = row[i];
          if (value.size() > colLenghts[i]) {
            colLenghts[i] = value.size();
          }
        }
      }

      for (Row &row : rows) {
        std::vector<std::string> cols;
        cols.reserve(cols.size());

        for (int i = 0; i < columns.size(); ++i) {
          Column &column = columns[i];
          if (column.lines == 0) {
            continue;
          }

          std::string e = row[i];
          int pad = colLenghts[i] - e.size() + 2;
          if (not column.separate_line && column.lines > 0) {
            e += std::string(pad, ' ');
            fmt::print(e);
          }

          cols.push_back(e);
        }

        fmt::print("\n");

        for (int i = 0; i < columns.size(); ++i) {
          Column &column = columns[i];
          if (not column.separate_line || cols[i].size() == 0) {
            continue;
          }

          fmt::print("  {}: {}\n", column.name, cols[i]);
        }
      }
    }
  };
}  // namespace fc::cli
