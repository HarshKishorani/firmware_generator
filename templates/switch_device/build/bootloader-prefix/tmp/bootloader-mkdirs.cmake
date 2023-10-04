# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/esp/esp-idf/components/bootloader/subproject"
  "F:/Firmware/code_generator/template/switch_device/build/bootloader"
  "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix"
  "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix/tmp"
  "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix/src/bootloader-stamp"
  "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix/src"
  "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "F:/Firmware/code_generator/template/switch_device/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
