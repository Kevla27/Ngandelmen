# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure")
  file(MAKE_DIRECTORY "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure")
endif()
file(MAKE_DIRECTORY
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/build"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure/tmp"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure/src/Uji_Coba_Integrasi_TZ_NS-stamp"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure/src"
  "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure/src/Uji_Coba_Integrasi_TZ_NS-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure/src/Uji_Coba_Integrasi_TZ_NS-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build/Debug/NonSecure/src/Uji_Coba_Integrasi_TZ_NS-stamp${cfgdir}") # cfgdir has leading slash
endif()
