/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/run.hpp"

namespace fc::cli::test {
  struct App : Empty {
    struct Args {
      CLI_OPTIONAL("mod", "", int) mod;

      CLI_OPTS() {
        Opts opts;
        mod(opts);
        return opts;
      }
    };
  };

  struct App_add {
    struct Args {
      CLI_OPTIONAL("a", "", int) a;
      CLI_OPTIONAL("b", "", int) b;

      CLI_OPTS() {
        Opts opts;
        a(opts);
        b(opts);
        return opts;
      }
    };
    CLI_RUN() {
      auto &app{argm.of<App>()};
      auto add{*args.a + *args.b};
      if (app.mod) {
        add = add % *app.mod;
      }
      fmt::print("add = {}\n", add);
    }
  };

  struct App_sum : Empty {
    CLI_RUN() {
      auto &app{argm.of<App>()};
      int sum{0};
      for (auto &arg : argv) {
        sum += cliArgv<int>(arg, "nums");
      }
      if (app.mod) {
        sum = sum % *app.mod;
      }
      fmt::print("sum = {}\n", sum);
    }
  };

  const auto _tree{tree<App>({
      {"add", tree<App_add>()},
      {"sum", tree<App_sum>()},
      {"math",
       tree<Group>({
           {"add", tree<App_add>()},
           {"sum", tree<App_sum>()},
       })},
  })};

  void main() {
    auto test{[](std::vector<std::string> argv) {
      fmt::print("test: {}\n", fmt::join(argv, " "));
      run("cli-test", _tree, argv);
      fmt::print("\n");
    }};
    test({});
    test({"-h"});
    test({"--help"});
    test({"add", "-h"});
    test({"add", "--a", "13", "--b", "14"});
    test({"math", "-h"});
    test({"math", "sum", "-h"});
    test({"--mod", "20", "math", "sum", "11", "22", "33"});
  }
}  // namespace fc::cli::test

int main() {
  fc::cli::test::main();
}
