target_sources(firmware PRIVATE manual_belt_tuning_wizard.cpp)

if(HAS_AUTOMATIC_BELT_TUNING)
  target_sources(firmware PRIVATE belt_resonance.cpp)
endif()
