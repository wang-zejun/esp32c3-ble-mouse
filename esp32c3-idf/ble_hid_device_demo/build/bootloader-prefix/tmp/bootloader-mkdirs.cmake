# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/esp/esp-idf/components/bootloader/subproject"
  "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader"
  "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader-prefix"
  "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader-prefix/tmp"
  "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader-prefix/src"
  "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/75234/Desktop/esp32c3-idf/ble_hid_device_demo/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
