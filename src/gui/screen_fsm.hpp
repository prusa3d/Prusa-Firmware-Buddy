#pragma once

#include "screen.hpp"
#include <common/static_storage.hpp>
#include "window_header.hpp"
#include "window_frame.hpp"
#include <common/fsm_base_types.hpp>
#include <IDialogMarlin.hpp>

namespace common_frames {
// Blank screen is often needed to avoid short flicker of the lower screen when switching from (different FSM's) dialog to ScreenFSM
class Blank final {
public:
    explicit Blank([[maybe_unused]] window_t *parent) {}
};
template <typename T>
concept is_update_callable = std::is_invocable_v<decltype(&T::update), T &, fsm::PhaseData>;

}; // namespace common_frames

template <auto Phase, class Frame, auto... constructor_args>
struct FrameDefinition {
    using FrameType = Frame;
    static constexpr auto phase = Phase;
};

template <class Storage, class... T>
struct FrameDefinitionList {
    static void create_frame(Storage &storage, auto phase, auto... args) {
        auto f = [&]<auto phase_, typename Frame, auto... constructor_args>(FrameDefinition<phase_, Frame, constructor_args...>) {
            static_assert(
                !std::is_base_of_v<window_t, Frame> || requires { Frame::needs_to_inherit_from_window_t; }, "Frames should not inherit from window_t (ideally from anything), it increases flash usage");

            if (phase == phase_) {
                storage.template create<Frame>(args..., constructor_args...);
            }
        };
        (f(T {}), ...);
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

template <typename Parent, size_t FrameStorageSize>
class WindowFSM : public Parent {
public:
    using FrameStorage = StaticStorageUnsafe<FrameStorageSize>;

    template <typename... Args>
    WindowFSM(Rect16 inner_frame_rect, Args &&...args)
        : Parent(std::forward<Args>(args)...)
        , inner_frame { this, inner_frame_rect } {
        this->ClrMenuTimeoutClose();
        this->CaptureNormalWindow(inner_frame);
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

protected:
    window_frame_t inner_frame;
    FrameStorage frame_storage;
    fsm::BaseData fsm_base_data;

    virtual void create_frame() = 0;
    virtual void destroy_frame() = 0;
    virtual void update_frame() = 0;
};

class ScreenFSM : public WindowFSM<screen_t, 2016> {

public:
    ScreenFSM(const char *header_txt, Rect16 inner_frame_rect = GuiDefaults::RectScreenNoHeader)
        : WindowFSM(inner_frame_rect)
        , header { this, _(header_txt) } {}

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
};

class DialogFSM : public WindowFSM<IDialogMarlin, 1124> {

public:
    DialogFSM(fsm::BaseData data)
        : WindowFSM(GuiDefaults::RectScreenNoHeader, GuiDefaults::RectScreenNoHeader) {
        fsm_base_data = data;
    }
};
