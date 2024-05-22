#pragma once

#include "gui.hpp"
#include "screen.hpp"
#include "window_header.hpp"
#include <common/fsm_base_types.hpp>
#include "esp_frame_text.hpp"
#include "esp_frame_progress.hpp"
#include "esp_frame_qr.hpp"
#include "static_alocation_ptr.hpp"

class ScreenESP : public AddSuperWindow<screen_t> {
    using mem_space = std::aligned_union<0, ESPFrameText, ESPFrameProgress, ESPFrameQR>::type;
    mem_space all_tests;

    // safer than make_static_unique_ptr, checks storage size
    template <class T, class... Args>
    static_unique_ptr<ESPFrame> makePtr(Args &&...args) {
        static_assert(sizeof(T) <= sizeof(all_tests), "Error esp part does not fit");
        return make_static_unique_ptr<T>(&all_tests, std::forward<Args>(args)...);
    }

    static_unique_ptr<ESPFrame> ptr;

private:
    static constexpr const char *en_esp = N_("WI-FI MODULE");

    window_header_t header;
    static ScreenESP *ths;

    ESPParts part_current;
    ESPParts part_previous;

public:
    ScreenESP();
    ~ScreenESP();
    static ScreenESP *GetInstance();
    void Change(fsm::BaseData data);
};
