#pragma once

#include <cstdint>
#include <optional>
#include <tool_index.hpp>


struct M109Flags {
  int16_t target_temp;
  bool wait_heat = true;            // Wait only when heating
  bool wait_heat_or_cool = false;   // Wait for heating or cooling
  bool autotemp = false;         // Use print fan to assist cooling/heating during wait
  std::optional<float> display_temp = std::nullopt; // If nullopt -> display nozzle temp
  std::optional<float> early_return_temperature = std::nullopt; // If set, wait for this temperature without changing the target; capped by the regular target wait (M109 `C`)
};

/**
* @brief Set extruder temperature and wait.
*
* @param flags @see M109Flags
*/
void M109_no_parser(PhysicalToolIndex tool, const M109Flags& flags = {});
