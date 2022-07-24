# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jirka/programovani/pico/pico-sdk/tools/pioasm"
  "/home/jirka/programovani/pico/Picopod/build/pioasm"
  "/home/jirka/programovani/pico/Picopod/build/pico-sdk/src/rp2_common/cyw43_driver/pioasm"
  "/home/jirka/programovani/pico/Picopod/build/pico-sdk/src/rp2_common/cyw43_driver/pioasm/tmp"
  "/home/jirka/programovani/pico/Picopod/build/pico-sdk/src/rp2_common/cyw43_driver/pioasm/src/PioasmBuild-stamp"
  "/home/jirka/programovani/pico/Picopod/build/pico-sdk/src/rp2_common/cyw43_driver/pioasm/src"
  "/home/jirka/programovani/pico/Picopod/build/pico-sdk/src/rp2_common/cyw43_driver/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jirka/programovani/pico/Picopod/build/pico-sdk/src/rp2_common/cyw43_driver/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
