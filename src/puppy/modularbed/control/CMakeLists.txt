target_sources(
  firmware
  PRIVATE AutoconfLogic.cpp
          ControlLogic.cpp
          ControlTask.cpp
          HeatbedletInfo.cpp
          IIR1Filter.cpp
          MeasurementCalculator.cpp
          MeasurementLogic.cpp
          MovingAverageFilter.cpp
          PWMLogic.cpp
          StateLogic.cpp
  )
