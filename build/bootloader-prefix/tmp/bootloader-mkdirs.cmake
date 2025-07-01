# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/Softwaves/Espressif/frameworks/esp-idf-v5.3.1/components/bootloader/subproject"
  "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader"
  "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix"
  "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix/tmp"
  "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix/src/bootloader-stamp"
  "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix/src"
  "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "G:/HCMUT/HK232/Internship-Exp/General-Project/Bluetooth_POC/Code_bluetooth_esp32s3/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
