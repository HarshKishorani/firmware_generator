# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "1_90c96c08.bin"
  "1_90c96c08.map"
  "aws-root-ca.pem.S"
  "bootloader\\bootloader.bin"
  "bootloader\\bootloader.elf"
  "bootloader\\bootloader.map"
  "certificate.pem.crt.S"
  "config\\sdkconfig.cmake"
  "config\\sdkconfig.h"
  "data.json.S"
  "esp-idf\\esptool_py\\flasher_args.json.in"
  "esp-idf\\mbedtls\\x509_crt_bundle"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "https_certificate.pem.S"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "private.pem.key.S"
  "project_elf_src_esp32.c"
  "x509_crt_bundle.S"
  )
endif()
