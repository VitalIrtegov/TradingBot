# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\TradingBot_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\TradingBot_autogen.dir\\ParseCache.txt"
  "TradingBot_autogen"
  )
endif()
