if(HAS_CHAMBER_API)
  target_sources(firmware PRIVATE menu_items_chamber.cpp)
endif()

if(HAS_CHAMBER_FILTRATION_API)
  target_sources(firmware PRIVATE menu_items_chamber_filtration.cpp)
endif()

if(XBUDDY_EXTENSION_VARIANT_IS_STANDARD)
  target_sources(firmware PRIVATE menu_items_xbuddy_extension.cpp)
endif()

if(HAS_MMU2)
  target_sources(firmware PRIVATE menu_items_hw_mmu.cpp)
endif()

if(HAS_SWITCHABLE_AUTO_RETRACT)
  target_sources(firmware PRIVATE menu_items_auto_retract.cpp)
endif()
