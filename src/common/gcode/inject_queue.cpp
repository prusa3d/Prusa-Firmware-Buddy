#include "inject_queue.hpp"

#include "gcode_loader.hpp"
#include <str_utils.hpp>

InjectQueue inject_queue; // instance

static GCodeLoader loader; // loader instance for async gcode loading from the InjectQueue

bool InjectQueue::try_push(InjectQueueRecord record) {
    return queue.try_put(record);
}

std::expected<const char *, InjectQueue::GetGCodeError> InjectQueue::get_gcode() {
    auto loader_result = loader.get_result();

    if (loader_result.has_value()) {
        loader.reset();
        return loader_result.value();

    } else if (loader_result.error() == GCodeLoader::BufferState::buffering) {
        return std::unexpected(GetGCodeError::buffering);

    } else if (loader_result.error() == GCodeLoader::BufferState::error) {
        loader.reset();
        return std::unexpected(GetGCodeError::loading_aborted);
    }

    // If we've reached here, we need to check the queue and do something
    InjectQueueRecord item;
    if (!queue.try_get(item)) {
        return std::unexpected(GetGCodeError::empty);
    }

    // The item is a literal -> just return the literal
    if (const auto *val = std::get_if<GCodeLiteral>(&item)) {
        return val->gcode;
    }

    if (const auto button = std::get_if<GCodeMacroButton>(&item)) {
        char filepath_buf[sizeof("btn_XXX")];
        StringBuilder filepath(filepath_buf);
        filepath.append_printf("btn_%hu", button->button);
        if (!filepath.is_ok()) {
            return std::unexpected(GetGCodeError::loading_aborted);
        }
        loader.load_gcode(filepath_buf);
    }

    if (const auto file = std::get_if<GCodeFilename>(&item)) {
        loader.load_gcode(file->name, file->fallback);
    }

    return std::unexpected(GetGCodeError::buffering);
}
