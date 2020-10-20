#Test Vectors
Tests runner for [filecoin-project/test-vectors]( 
https://github.com/filecoin-project/test-vectors).

Tests enhanced with additional logging defined in `dvm`.
Set environment variable to specify output log file:

`DVM_LOG=/path/to/cpp-logs.txt`

The similar logs for lotus implementation in Go can be found at https://github.com/turuslan/lotus/tree/dvm10.

To run a single tests, add the following code:
```
#include "dvm"

...

auto search(bool enabled) {
  ...

  // enable logging
  fc::dvm::logging = true;
  fc::dvm::logger->flush_on(spdlog::level::info);
  // set single test
  auto add{[&](auto s) {
    vectors.push_back(MessageVector::read(kCorpusRoot + "/" + s + ".json"));
  }};
  add("path/to/test/in/vector");
  return vectors;

  ...
}
```
