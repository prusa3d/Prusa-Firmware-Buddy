add_library(
  TMCStepper
  TMCStepper/src/source/CHOPCONF.cpp
  TMCStepper/src/source/COOLCONF.cpp
  TMCStepper/src/source/DRV_STATUS.cpp
  TMCStepper/src/source/GCONF.cpp
  TMCStepper/src/source/IHOLD_IRUN.cpp
  TMCStepper/src/source/PWMCONF.cpp
  TMCStepper/src/source/SERIAL_SWITCH.cpp
  TMCStepper/src/source/SW_SPI.cpp
  TMCStepper/src/source/TMC2130Stepper.cpp
  TMCStepper/src/source/TMC2208Stepper.cpp
  TMCStepper/src/source/TMC2209Stepper.cpp
  TMCStepper/src/source/TMC_lock.cpp
  TMCStepper/src/source/TMCStepper.cpp
  )

target_include_directories(TMCStepper PUBLIC TMCStepper/src)

target_link_libraries(TMCStepper PUBLIC Arduino::Core)
target_link_libraries(TMCStepper PRIVATE Marlin) # for prusa toolchanger

add_library(Arduino::TMCStepper ALIAS TMCStepper)
