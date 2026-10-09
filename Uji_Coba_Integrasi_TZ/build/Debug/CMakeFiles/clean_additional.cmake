# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "G:\\PPPP\\Day_Sekian\\Uji_Coba_Integrasi_TZ\\NonSecure\\build"
  "G:\\PPPP\\Day_Sekian\\Uji_Coba_Integrasi_TZ\\Secure\\build"
  )
endif()
