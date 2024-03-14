target_sources(
  firmware
  PRIVATE avr/cpu.cpp
          avr/usart.cpp
          avr/shr16.cpp
          avr/eeprom.cpp
          avr/timers.cpp
          tmc2130.cpp
          adc.cpp
          avr/spi.cpp
          avr/watchdog.cpp
  )
