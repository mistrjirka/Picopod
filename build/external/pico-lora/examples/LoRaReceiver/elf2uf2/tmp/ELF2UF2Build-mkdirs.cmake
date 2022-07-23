# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jirka/programovani/pico/pico-sdk/tools/elf2uf2"
  "/home/jirka/programovani/pico/Picopod/build/elf2uf2"
  "/home/jirka/programovani/pico/Picopod/build/external/pico-lora/examples/LoRaReceiver/elf2uf2"
  "/home/jirka/programovani/pico/Picopod/build/external/pico-lora/examples/LoRaReceiver/elf2uf2/tmp"
  "/home/jirka/programovani/pico/Picopod/build/external/pico-lora/examples/LoRaReceiver/elf2uf2/src/ELF2UF2Build-stamp"
  "/home/jirka/programovani/pico/Picopod/build/external/pico-lora/examples/LoRaReceiver/elf2uf2/src"
  "/home/jirka/programovani/pico/Picopod/build/external/pico-lora/examples/LoRaReceiver/elf2uf2/src/ELF2UF2Build-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jirka/programovani/pico/Picopod/build/external/pico-lora/examples/LoRaReceiver/elf2uf2/src/ELF2UF2Build-stamp/${subDir}")
endforeach()
