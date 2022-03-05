/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace fc {
  // TODO: string character count (e.g. ansi escape codes, or unicode)
  struct TableWriter {
    struct Column {
      std::string name;
      // note: (l)eft, (r)ight, (n)ewline
      char align;
      // NOLINTNEXTLINE(google-explicit-constructor)
      Column(const char *name, char align = 'l') : name{name}, align{align} {
        assert(align == 'l' || align == 'r' || align == 'n');
      }
    };

    using Rows = std::list<std::vector<std::string>>;

    struct Row {
      const TableWriter &table;
      Rows::iterator row;

      std::string &operator[](std::string_view name) {
        return row->at(table.index(name));
      }
    };

    std::vector<Column> columns;
    Rows rows;

    explicit TableWriter(std::initializer_list<Column> columns)
        : columns{columns} {}

    size_t index(std::string_view name) const {
      const auto it{std::find_if(
          columns.begin(), columns.end(), [&](const Column &column) {
            return column.name == name;
          })};
      assert(it != columns.end());
      return it - columns.begin();
    }

    Row row() {
      return Row{*this, rows.emplace(rows.end(), columns.size())};
    }

    void write(std::ostream &os) const {
      std::vector<size_t> count;
      count.resize(columns.size());
      std::vector<size_t> width;
      width.resize(columns.size());
      for (const auto &row : rows) {
        for (size_t i{0}; i < columns.size(); ++i) {
          if (!row.at(i).empty()) {
            ++count.at(i);
            width.at(i) = std::max(width.at(i), row.at(i).size());
          }
        }
      }
      std::vector<std::string> header;
      for (size_t i{0}; i < columns.size(); ++i) {
        width.at(i) = std::max(width.at(i), columns.at(i).name.size());
        header.emplace_back(columns.at(i).name);
      }
      const auto writeColumns{[&](const std::vector<std::string> &row) {
        auto first{true};
        for (size_t i{0}; i < columns.size(); ++i) {
          if (columns.at(i).align == 'n' || count.at(i) == 0) {
            continue;
          }
          if (!first) {
            // note: column separator
            os << "  ";
          }
          first = false;
          if (columns.at(i).align == 'l') {
            os << row.at(i);
          }
          for (size_t pad{width.at(i) - row.at(i).size()}; pad != 0; --pad) {
            os << ' ';
          }
          if (columns.at(i).align == 'r') {
            os << row.at(i);
          }
        }
        os << "\n";
      }};
      writeColumns(header);
      for (const auto &row : rows) {
        writeColumns(row);
        for (size_t i{0}; i < columns.size(); ++i) {
          if (columns.at(i).align == 'n' && !row.at(i).empty()) {
            os << "  " << columns.at(i).name << ": " << row.at(i) << "\n";
          }
        }
      }
    }
  };
}  // namespace fc
