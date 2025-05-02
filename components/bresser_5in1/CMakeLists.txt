# esphome-bresser-5in1 CMakeLists.txt
esphome_add_library(
  SRC bresser_5in1.cpp
  INCLUDE bresser_5in1.h
)

# Include the RadioLib library
external_libraries:
  - RadioLib
