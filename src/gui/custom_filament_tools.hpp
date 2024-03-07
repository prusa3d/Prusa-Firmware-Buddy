#pragma once
namespace custom_filament_tools {

enum class CustomFilamentTemperatures : uint8_t {
    nozzle = 0,
    nozzle_preheat = 1,
    heatbed = 2
};

enum class CustomFilamentSlots : uint8_t {
    CUSTOM_1 = 0,
    CUSTOM_2 = 1,
    CUSTOM_3 = 2,
    CUSTOM_4 = 3,
    END
};

inline constexpr size_t max_filament_name_size { 9 };
inline constexpr size_t max_filament_slots { size_t(CustomFilamentSlots::END) };

#define FILAMENT_VAR_MSK(n_id) ((uint32_t)1 << (n_id))

typedef enum {
    FILAMENT_VAR_NAME_1, // char[8+1]
    FILAMENT_VAR_NAME_2, // char[8+1]
    FILAMENT_VAR_NAME_3, // char[8+1]
    FILAMENT_VAR_NAME_4, // char[8+1]
    FILAMENT_VAR_NOZZLE_1, // int16_t
    FILAMENT_VAR_NOZZLE_2, // int16_t
    FILAMENT_VAR_NOZZLE_3, // int16_t
    FILAMENT_VAR_NOZZLE_4, // int16_t
    FILAMENT_VAR_NOZZLE_PREHEAT_1, // int16_t
    FILAMENT_VAR_NOZZLE_PREHEAT_2, // int16_t
    FILAMENT_VAR_NOZZLE_PREHEAT_3, // int16_t
    FILAMENT_VAR_NOZZLE_PREHEAT_4, // int16_t
    FILAMENT_VAR_HEATBED_1, // int16_t
    FILAMENT_VAR_HEATBED_2, // int16_t
    FILAMENT_VAR_HEATBED_3, // int16_t
    FILAMENT_VAR_HEATBED_4, // int16_t
} FILAMENT_VAR_t;

struct CustomFilamentConfig {
    char name[max_filament_slots][max_filament_name_size] = { "", "", "", "" };
    int16_t nozzle[max_filament_slots] = { 210, 210, 210, 210 };
    int16_t nozzle_preheat[max_filament_slots] = { 170, 170, 170, 170 };
    int16_t heatbed[max_filament_slots] = { 60, 60, 60, 60 };
    uint32_t mask = 0;
};

void SetCurrentSlot(const CustomFilamentSlots slot);
CustomFilamentSlots GetCurrentSlot();
int16_t GetSlotTemp(const CustomFilamentTemperatures setting);
void SetSlotTemp(const CustomFilamentTemperatures setting, const int16_t temp);
const char *GetSlotName();
const char *GetSlotName(const CustomFilamentSlots slot);
void SetSlotName(char *name);
bool load_cfg_from_ini();
CustomFilamentSlots IndexToSlot(const size_t index);
uint8_t SlotToIndex(CustomFilamentSlots slot);
uint8_t TempToIndex(CustomFilamentTemperatures temp);

} // namespace custom_filament_tools
