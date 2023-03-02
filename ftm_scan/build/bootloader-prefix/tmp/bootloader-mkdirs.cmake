# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/emanuele/esp/esp-idf/components/bootloader/subproject"
  "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader"
  "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix"
  "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix/tmp"
  "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix/src"
  "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/emanuele/esp/esp-idf/examples/wifi/ftm_scan/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
