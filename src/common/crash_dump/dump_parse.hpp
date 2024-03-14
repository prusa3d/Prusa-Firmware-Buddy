#pragma once

namespace crash_dump {

/**
 * @brief Parses CrashDump data to display display blue screen of death info
 */
class CrashCatcherDumpParser {
public:
    struct Registers_t {
        uint32_t R[13];
        uint32_t SP;
        uint32_t LR;
        uint32_t PC;
        uint32_t PSR;
        uint32_t MSP;
        uint32_t PSP;
        uint32_t exceptionPSR;
    };

    CrashCatcherDumpParser();

    /**
     * Load data that were on addres "from" when crash was dumped
     * @param to Pointer where data will be saved
     * @param from Data from dump we want to load (can be RAM or registers, whatever memory region that is inside dump)
     * @param size Size of data to copy
     * @return True when data found, false when not
     */
    bool load_data(uint8_t *to, uintptr_t from, size_t size);

    /**
     * Loads registers at time of dump (see Registers_t)
     */
    void load_registers(Registers_t &registers);

private:
    uint32_t dump_size; ///< Total size of dump
    uint32_t flags; ///< loaded flags of dump

    // sizes of each dump component
    inline static constexpr uint32_t SIGNATURE_SIZE = 4;
    inline static constexpr uint32_t FLAGS_SIZE = sizeof(uint32_t);
    inline static constexpr uint32_t REGISTERS_SIZE = sizeof(Registers_t);
    inline static constexpr uint32_t FLOATING_POINT_REGISTERS_SIZE = (32 + 1) * sizeof(uint32_t);

    // offset of each dump component
    inline static constexpr uint32_t FLAGS_OFFSET = SIGNATURE_SIZE;
    inline static constexpr uint32_t REGISTERS_OFFSET = FLAGS_OFFSET + FLAGS_SIZE;
    inline static constexpr uint32_t DATA_BLOCKS_OFFSET_WITH_FP = REGISTERS_OFFSET + REGISTERS_SIZE + FLOATING_POINT_REGISTERS_SIZE;
    inline static constexpr uint32_t DATA_BLOCKS_OFFSET_WITHOUT_FP = REGISTERS_OFFSET + REGISTERS_SIZE;
};

} // namespace crash_dump
