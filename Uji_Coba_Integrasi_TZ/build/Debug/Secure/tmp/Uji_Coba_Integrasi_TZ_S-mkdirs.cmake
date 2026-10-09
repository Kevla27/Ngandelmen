# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure")
  file(MAKE_DIRECTORY "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure")
endif()
file(MAKE_DIRECTORY
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure/build"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure/tmp"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure/src/Uji_Coba_Integrasi_TZ_S-stamp"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure/src"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure/src/Uji_Coba_Integrasi_TZ_S-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure/src/Uji_Coba_Integrasi_TZ_S-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/Secure/src/Uji_Coba_Integrasi_TZ_S-stamp${cfgdir}") # cfgdir has leading slash
endif()
