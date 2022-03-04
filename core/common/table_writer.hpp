/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#include <string>
#include <vector>
#include <map>

class TableWriter {
  struct Column {
    std::string name;
    bool separate_line;
    size_t lines = 0;
  };
  using Row = std::map<size_t, std::string>;

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

  explicit TableWriter(std::vector<Column> cols) : columns(std::move(cols)) {}

  void write(std::map<std::string, std::string> range) {
    std::map<size_t, std::string> by_column_id;
    bool flag = false;

    while (not flag) {
      for (auto &[columnName, value] : range) {
        for (size_t i = 0; i < columns.size(); ++i) {
          Column &column = columns[i];
          if (column.name == columnName) {
            by_column_id[i] = std::move(value);
            range.erase(columnName);
            column.lines++;
            flag = true;
            break;
          }
        }

        if (flag) {
          break;
        }
        by_column_id[columns.size()] = std::move(value);
        range.erase(columnName);
        columns.push_back(
            Column{.name = columnName, .separate_line = false, .lines = 1});
      }
      flag = not flag;
    }

    rows.push_back(by_column_id);
  }

  void flush() {
    std::vector<size_t> col_lengths(columns.size());

    std::map<size_t, std::string> header;
    for (size_t i = 0; i < columns.size(); ++i) {
      const Column &column = columns[i];
      if (column.separate_line) {
        continue;
      }
      header[i] = column.name;
    }

    rows.insert(rows.begin(), header);

    for (size_t i = 0; i < columns.size(); ++i) {
      const Column &column = columns[i];
      if (column.lines == 0) {
        continue;
      }

      for (auto &row : rows) {
        if (row.find(i) == row.end()) {
          continue;
        }
        std::string value = row[i];
        if (value.size() > col_lengths[i]) {
          col_lengths[i] = value.size();
        }
      }
    }

    for (Row &row : rows) {
      std::vector<std::string> cols(columns.size());

      for (size_t i = 0; i < columns.size(); ++i) {
        const Column &column = columns[i];
        if (column.lines == 0) {
          continue;
        }

        std::string for_print = row[i];
        const size_t pad = col_lengths[i] + 2 - for_print.size();
        if (not column.separate_line && column.lines > 0) {
          for_print += std::string(pad, ' ');
          fmt::print(for_print);
        }

        cols.push_back(for_print);
      }

      fmt::print("\n");

      for (size_t i = 0; i < columns.size(); ++i) {
        const Column &column = columns[i];
        if (not column.separate_line || cols[i].empty()) {
          continue;
        }

        fmt::print("  {}: {}\n", column.name, cols[i]);
      }
    }
  }
};