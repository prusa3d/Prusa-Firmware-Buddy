#pragma once

#include <option/has_chamber_vents.h>
#include <option/has_chamber_api.h>
#include <option/has_auto_retract.h>
#include <option/has_print_sheet_detection.h>
#include <option/has_spool_join.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_indx.h>
#include <option/has_nozzle_cleaner.h>
#include <option/has_nozzle_cleaner_lite.h>

#include "print_status_message_data.hpp"
#include <inc/MarlinConfigPre.h>

struct PrintStatusMessage {
    enum Type : uint8_t {
        none,

        /// Custom, performatted text
        custom,

        homing,
        homing_retrying,
        homing_refining,
        recalibrating_home,
        calibrating_axis,
        probing_bed,
        additional_probing,

        dwelling,

        absorbing_heat,

        waiting_for_hotend_temp,
        waiting_for_bed_temp,

#if ENABLED(PROBE_CLEANUP_SUPPORT)
        nozzle_cleaning,
#endif
#if HAS_PRINT_SHEET_DETECTION()
        detecting_steel_sheet,
#endif
#if HAS_SPOOL_JOIN()
        spool_joined,
        joining_spool,
#endif
#if HAS_CHAMBER_API()
        waiting_for_chamber_temp,
#endif
#if HAS_AUTO_RETRACT()
        auto_retracting,
#endif
#if HAS_CHAMBER_VENTS()
        opening_chamber_vents,
        closing_chamber_vents,
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        tool_offset_calibrating,
#endif
#if HAS_NOZZLE_CLEANER()
        nozzle_cleaner,
#endif
#if HAS_NOZZLE_CLEANER_LITE()
        nozzle_cleaner_lite,
#endif
        _cnt
    };
    using Data = PrintStatusMessageData;

    template <Type type_, typename Data_>
    struct TypeRecord {
        static constexpr Type type = type_;
        using Data = Data_;

        template <typename T>
        consteval auto operator<<(T v) const {
            // We're putting the needle (the type we're looking for) as the rightmost operand in the fold expression
            // So if our type (the left operand) matches type other type, it means that we've got a hit and we should return this record.
            if constexpr (T::type == type) {
                return *this;
            } else {
                return v;
            }
        }
    };

    template <typename... Records_>
    struct TypeRecordList {

        /// \returns TypeRecord() of the type matching type_ or
        template <Type type>
        static consteval auto get_() {
            // This is quite C++ magical, but basically it returns instance of TypeRecord with appropriate for the given type
            constexpr auto needle = TypeRecord<type, void>();
            constexpr auto r = (Records_() << ... << needle);
            static_assert(!std::is_same_v<decltype(r), decltype(needle)>, "Message type missing from the record list");
            return r;
        }
    };

    using type_record_list = TypeRecordList<
        TypeRecord<Type::custom, PrintStatusMessageDataCustom>,
        TypeRecord<Type::homing, std::monostate>,
        TypeRecord<Type::homing_retrying, PrintStatusMessageDataAxisProgress>,
        TypeRecord<Type::homing_refining, PrintStatusMessageDataAxisProgress>,
        TypeRecord<Type::recalibrating_home, std::monostate>,
        TypeRecord<Type::calibrating_axis, PrintStatusMessageDataAxisProgress>,
        TypeRecord<Type::probing_bed, PrintStatusMessageDataProgress>,
        TypeRecord<Type::additional_probing, PrintStatusMessageDataProgress>,
        TypeRecord<Type::dwelling, PrintStatusMessageDataProgress>,
        TypeRecord<Type::absorbing_heat, PrintStatusMessageDataProgress>,
        TypeRecord<Type::waiting_for_hotend_temp, PrintStatusMessageDataToolProgress>,
        TypeRecord<Type::waiting_for_bed_temp, PrintStatusMessageDataProgress>,
#if ENABLED(PROBE_CLEANUP_SUPPORT)
        TypeRecord<Type::nozzle_cleaning, std::monostate>,
#endif
#if HAS_PRINT_SHEET_DETECTION()
        TypeRecord<Type::detecting_steel_sheet, std::monostate>,
#endif
#if HAS_SPOOL_JOIN()
        TypeRecord<Type::spool_joined, std::monostate>,
        TypeRecord<Type::joining_spool, std::monostate>,
#endif
#if HAS_CHAMBER_API()
        TypeRecord<Type::waiting_for_chamber_temp, PrintStatusMessageDataProgress>,
#endif
#if HAS_AUTO_RETRACT()
        TypeRecord<Type::auto_retracting, PrintStatusMessageDataProgress>,
#endif
#if HAS_CHAMBER_VENTS()
        TypeRecord<Type::opening_chamber_vents, std::monostate>,
        TypeRecord<Type::closing_chamber_vents, std::monostate>,
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        TypeRecord<Type::tool_offset_calibrating, PrintStatusMessageDataToolProgress>,
#endif
#if HAS_NOZZLE_CLEANER()
        TypeRecord<Type::nozzle_cleaner, std::monostate>,
#endif
#if HAS_NOZZLE_CLEANER_LITE()
        TypeRecord<Type::nozzle_cleaner_lite, std::monostate>,
#endif

        TypeRecord<Type::none, std::monostate>>;

    template <Type type>
    using TypeRecordOf = decltype(type_record_list::get_<type>());

    PrintStatusMessage() = default;

    /// This function enforces that the data type matches the one expected from the type_record_list
    template <Type type>
    static PrintStatusMessage make(const TypeRecordOf<type>::Data &data) {
        return PrintStatusMessage(type, data);
    }

    Type type = Type::none;
    Data data;

    bool operator==(const PrintStatusMessage &) const = default;

private:
    /// Private only - use make() which does data type checking
    PrintStatusMessage(Type type, const Data &data)
        : type(type)
        , data(data) {}
};

struct PrintStatusMessageRecord {
    PrintStatusMessage message;
    uint32_t id = 0;

    explicit operator bool() const {
        return message.type != PrintStatusMessage::none;
    }

    bool operator==(const PrintStatusMessageRecord &) const = default;
};
