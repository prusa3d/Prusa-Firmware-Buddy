#pragma once

#include "screen.hpp"
#include <common/static_storage.hpp>
#include "window_header.hpp"
#include "window_frame.hpp"
#include <common/fsm_base_types.hpp>

namespace common_frames {
// Blank screen is often needed to avoid short flicker of the lower screen when switching from (different FSM's) dialog to ScreenFSM
class Blank final {
public:
    explicit Blank([[maybe_unused]] window_t *parent) {}
};
template <typename T>
concept is_update_callable = std::is_invocable_v<decltype(&T::update), T &, fsm::PhaseData>;

}; // namespace common_frames

template <auto Phase, class Frame>
struct FrameDefinition {
    using FrameType = Frame;
    static constexpr auto phase = Phase;
};

template <class Storage, class... T>
struct FrameDefinitionList {
    template <class F>
    using FrameType = typename F::FrameType;

    static_assert(Storage::template has_enough_space_for<FrameType<T>...>());

    static void create_frame(Storage &storage, auto phase, auto... args) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template create<typename FD::FrameType>(args...);
            }
        };
        (f.template operator()<T>(), ...);
    }

    static void destroy_frame(Storage &storage, auto phase) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template destroy<typename FD::FrameType>();
            }
        };
        (f.template operator()<T>(), ...);
    }

    static void update_frame(Storage &storage, auto phase, const fsm::PhaseData &data) {
        auto f = [&]<typename FD> {
            if constexpr (common_frames::is_update_callable<typename FD::FrameType>) {
                if (phase == FD::phase) {
                    storage.template as<typename FD::FrameType>()->update(data);
                }
            }
        };
        (f.template operator()<T>(), ...);
    }
};

class ScreenFSM : public screen_t {
    static constexpr size_t frame_static_storage_size = 1280;

public:
    using FrameStorage = StaticStorage<frame_static_storage_size>;

    ScreenFSM(const char *header_txt, Rect16 inner_frame_rect)
        : screen_t()
        , header { this, _(header_txt) }
        , inner_frame { this, inner_frame_rect } {
        ClrMenuTimeoutClose();
    }

    void Change(fsm::BaseData new_fsm_base_data) {
        if (new_fsm_base_data.GetPhase() != fsm_base_data.GetPhase()) {
            destroy_frame();
            fsm_base_data = new_fsm_base_data;
            create_frame();
        } else {
            fsm_base_data = new_fsm_base_data;
        }
        update_frame();
    }

    virtual void InitState(screen_init_variant var) override {
        if (auto fsm_base_data = var.GetFsmBaseData()) {
            Change(*fsm_base_data);
        }
    }

    virtual screen_init_variant GetCurrentState() const override {
        screen_init_variant var;
        var.SetFsmBaseData(fsm_base_data);
        return var;
    }

protected:
    window_header_t header;
    window_frame_t inner_frame;
    FrameStorage frame_storage;
    fsm::BaseData fsm_base_data;

    virtual void create_frame() = 0;
    virtual void destroy_frame() = 0;
    virtual void update_frame() = 0;
};
