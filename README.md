# Perroht: Persistent Robin Hood Hash Table

## Getting Started

Perroht is a header-only C++ library that implements a persistent hash table designed to work with [Metall](https://github.com/LLNL/metall) and the allocators in [Boost.Interprocess](https://www.boost.org).
All core files exist under [Perroht/include/perroht](https://github.com/LLNL/Perroht/tree/main/include/perroht).
To use Perroht in a project, a C++17 compiler is required.

## Install or Build Tests

**Requirements**
- 
- CMake 3.20 or newer
- C++17 compiler (for building tests)

**Command Example:**

```shell
git clone git@github.com:LLNL/Perroht.git
cd Perroht
mkdir build && cd build

# To just install Perroht
cmake ../ -DCMAKE_INSTALL_PREFIX=<install_dir> -DJUST_INSTALL_PERROHT_HEADERS=ON
make install

# To build tests (requires C++17 compiler)
cmake ../ DBUILD_TEST=ON -DCMAKE_BUILD_TYPE=Debug -DBUILD_PERSISTENT_ALLOCATOR_TEST=<ON/OFF>
make -j
make test
```

## License

Perroht is distributed under the terms of the MIT license (see [LICENSE](./LICENSE) for details).

All new contributions must be made under this license.

SPDX-License-Identifier: MIT

## Release

LLNL-CODE-852126
