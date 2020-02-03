# Filecoin (cpp-filecoin)  

> C++17 implementation of blockchain based digital storage

Filecoin is a decentralized protocol described in [spec](https://filecoin-project.github.io/specs/)

## Dependencies

All C++ dependencies are managed using [Hunter](hunter.sh).
It uses cmake to download required libraries and do not require to download and install packages manually.
Target C++ compilers are:
* GCC 7.4
* Clang 6.0.1
* AppleClang 11.0

### Rust Installation

C++ Filecoin implementation uses Rust libraries as dependencies for BLS provider.
All you need to build these targets successfully - is Rust lang installed to your `${HOME}/.cargo/bin`.

You can do it manually following the instructions from [rust-lang.org](https://www.rust-lang.org/tools/install) or any other way.

Otherwise, CMake would not be able to compile Rust-related targets and will produce a warning about that during configuration.

## Clone

To clone repository execute
```
git clone https://github.com/filecoin-project/cpp-filecoin.git
cd cpp-filecoin
git submodule update --init --recursive
```

## Development
### Build cpp-filecoin

First build will likely take long time. However, you can cache binaries to [hunter-binary-cache](https://github.com/soramitsu/hunter-binary-cache) or even download binaries from the cache in case someone has already compiled project with the same compiler. To this end, you need to set up two environment variables:
```
GITHUB_HUNTER_USERNAME=<github account name>
GITHUB_HUNTER_TOKEN=<github token>
```
To generate github token follow the [instructions](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line). Make sure `read:packages` and `write:packages` permissions are granted (step 7 in instructions).

This project is can be built with

```
mkdir build && cd build
cmake -DCLANG_TIDY=ON ..
make -j
```

It is suggested to build project with clang-tidy checks, however if you wish to omit clang-tidy step, you can use `cmake ..` instead.

Tests can be run with: 
```
cd build
ctest
```

### CodeStyle

We follow [CppCoreGuidelines](https://github.com/isocpp/CppCoreGuidelines).

Please use provided [.clang-format](.clang-format) file to autoformat the code.  

## Maintenance

Maintainers: @kamilsa, @zuiris, @harrm, @masterjedy, @Mingela, @Warchant

Tickets: Can be opened in Github Issues.
