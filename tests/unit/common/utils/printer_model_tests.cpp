#include <catch2/catch_test_macros.hpp>

#include <common/printer_model.hpp>
#include <common/printer_model_data.hpp>

TEST_CASE("printer_model::getters") {
    const auto id_model = [](const char *id) {
        const auto result = PrinterModelInfo::from_id_str(id);
        CAPTURE(id);
        REQUIRE(result);
        return result->model;
    };
    const auto code_model = [](size_t code) {
        const auto result = PrinterModelInfo::from_gcode_check_code(code);
        CAPTURE(code);
        REQUIRE(result);
        return result->model;
    };

    CHECK(code_model(300) == PrinterModel::mk3);
    CHECK(id_model("MK3.5") == PrinterModel::mk3_5);
    CHECK(id_model("COREONE") == PrinterModel::coreone);
    CHECK(code_model(310) == PrinterModel::coreone);
    CHECK(id_model("COREONEOAK") == PrinterModel::coreone_oak);
    CHECK(code_model(380) == PrinterModel::coreone_oak);

    /// MMU variants
    CHECK(code_model(30260) == PrinterModel::mk4s);
    CHECK(id_model("MK4SMMU3") == PrinterModel::mk4s);
}

TEST_CASE("printer_model::compatibilities") {
    using Compatibility = PrinterGCodeCompatibilityReport;
    const auto compatibility = [](PrinterModel machine, PrinterModel gcode) -> Compatibility {
        return PrinterModelInfo::get(machine).gcode_compatibility_report(PrinterModelInfo::get(gcode));
    };

    CHECK(Compatibility().is_compatible == false);
    constexpr Compatibility fully_compatible { .is_compatible = true };

    // Each printer should be fully compatible with itself
    for (size_t i = 0; i < std::to_underlying(PrinterModel::_cnt); i++) {
        const PrinterModel model = static_cast<PrinterModel>(i);

        // Skip these, we don't care that the result is a bit wrong for them
        if (model == PrinterModel::mk3 || model == PrinterModel::mk3s) {
            continue;
        }

        CAPTURE(model);
        CHECK(compatibility(model, model) == fully_compatible);
    }

    // Some incompatibility checks
    CHECK(compatibility(PrinterModel::xl, PrinterModel::mk4) == Compatibility());
    CHECK(compatibility(PrinterModel::mini, PrinterModel::mk4) == Compatibility());

    // MK4 compatibility
    CHECK(compatibility(PrinterModel::mk3_9, PrinterModel::mk4) == fully_compatible);
    CHECK(compatibility(PrinterModel::mk4, PrinterModel::mk3_9) == fully_compatible);

    // MK3.5 and 3.5s should be fully compatible
    CHECK(compatibility(PrinterModel::mk3_5, PrinterModel::mk3_5s) == fully_compatible);
    CHECK(compatibility(PrinterModel::mk3_5s, PrinterModel::mk3_5) == fully_compatible);

    // MK3.5 family should not be compatible with MK4
    CHECK(!compatibility(PrinterModel::mk3_5, PrinterModel::mk4).is_compatible);
    CHECK(!compatibility(PrinterModel::mk4, PrinterModel::mk3_5).is_compatible);
    CHECK(!compatibility(PrinterModel::mk3_5s, PrinterModel::mk4s).is_compatible);
    CHECK(!compatibility(PrinterModel::mk3_5s, PrinterModel::mk4).is_compatible);
    CHECK(!compatibility(PrinterModel::mk4s, PrinterModel::mk3_5).is_compatible);

    // MK4 should not be able to run MK4S gcode
    CHECK(!compatibility(PrinterModel::mk4, PrinterModel::mk4s).is_compatible);

    // But MK4S should be able to run MK4 gcode in fan compatibility
    CHECK(compatibility(PrinterModel::mk4s, PrinterModel::mk4) == Compatibility { .is_compatible = true, .mk4_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::mk3_9s, PrinterModel::mk4) == Compatibility { .is_compatible = true, .mk4_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::mk3_9s, PrinterModel::mk3_9) == Compatibility { .is_compatible = true, .mk4_compatibility_mode = true });

    // Also check MK3 compat mode, only for the MK4 family
    CHECK(!compatibility(PrinterModel::mini, PrinterModel::mk3).is_compatible);
    CHECK(!compatibility(PrinterModel::xl, PrinterModel::mk3s).is_compatible);

    // COREONE tests
    CHECK(compatibility(PrinterModel::coreone, PrinterModel::coreone) == fully_compatible);
    CHECK(compatibility(PrinterModel::coreone, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true, .mk4_compatibility_mode = true, .chamber_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::coreone, PrinterModel::mk4) == Compatibility { .is_compatible = true, .mk4_compatibility_mode = true, .chamber_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::coreone, PrinterModel::mk4s) == Compatibility { .is_compatible = true, .chamber_compatibility_mode = true });
    CHECK(!compatibility(PrinterModel::coreone, PrinterModel::ix).is_compatible);
    CHECK(!compatibility(PrinterModel::coreone, PrinterModel::xl).is_compatible);
    CHECK(!compatibility(PrinterModel::coreone, PrinterModel::mk3_5).is_compatible);
    CHECK(!compatibility(PrinterModel::mk3_5, PrinterModel::coreone).is_compatible);
    CHECK(!compatibility(PrinterModel::mk4, PrinterModel::coreone).is_compatible);

    CHECK(compatibility(PrinterModel::mk3_5, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::mk3_9, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::mk4, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::mk3_9s, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true, .mk4_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::mk4s, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true, .mk4_compatibility_mode = true });

    // INDX_TODO: Implement compatibility tests

    // COREONE_OAK shares compatibility_group::coreone — same compat as Core One
    CHECK(compatibility(PrinterModel::coreone_oak, PrinterModel::coreone) == fully_compatible);
    CHECK(compatibility(PrinterModel::coreone, PrinterModel::coreone_oak) == fully_compatible);
    CHECK(compatibility(PrinterModel::coreone_oak, PrinterModel::mk3) == Compatibility { .is_compatible = true, .mk3_compatibility_mode = true, .mk4_compatibility_mode = true, .chamber_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::coreone_oak, PrinterModel::mk4) == Compatibility { .is_compatible = true, .mk4_compatibility_mode = true, .chamber_compatibility_mode = true });
    CHECK(compatibility(PrinterModel::coreone_oak, PrinterModel::mk4s) == Compatibility { .is_compatible = true, .chamber_compatibility_mode = true });
    CHECK(!compatibility(PrinterModel::coreone_oak, PrinterModel::ix).is_compatible);
    CHECK(!compatibility(PrinterModel::coreone_oak, PrinterModel::xl).is_compatible);
    CHECK(!compatibility(PrinterModel::coreone_oak, PrinterModel::mk3_5).is_compatible);
    CHECK(!compatibility(PrinterModel::mk4, PrinterModel::coreone_oak).is_compatible);

    // XLS compatibility tests
    CHECK(compatibility(PrinterModel::xls, PrinterModel::xls) == fully_compatible);
    CHECK(compatibility(PrinterModel::xls, PrinterModel::xl) == Compatibility { .is_compatible = true, .xl_compatibility_mode = true });
    CHECK(!compatibility(PrinterModel::xl, PrinterModel::xls).is_compatible);
    CHECK(!compatibility(PrinterModel::xls, PrinterModel::mk4).is_compatible);
    CHECK(!compatibility(PrinterModel::xls, PrinterModel::ix).is_compatible);

    // When increasing this number also add proper tests above
    static_assert(std::to_underlying(PrinterModel::_cnt) == 18);
}
