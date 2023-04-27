#include "homing.h"
#include "main_loop_stub.h"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/selector.h"

#include "../stubs/stub_motion.h"

void SimulateIdlerAndSelectorHoming(logic::CommandBase &cb) {
#if 0
    // do 5 steps until we trigger the simulated StallGuard
    for (uint8_t i = 0; i < 5; ++i) {
        main_loop();
        cb.Step();
    }

    mm::TriggerStallGuard(mm::Selector);
    mm::TriggerStallGuard(mm::Idler);
    main_loop();
    cb.Step();
    mm::motion.StallGuardReset(mm::Selector);
    mm::motion.StallGuardReset(mm::Idler);

    // now do a correct amount of steps of each axis towards the other end
    uint32_t idlerSteps = mm::unitToSteps<mm::I_pos_t>(config::idlerLimits.lenght);
    uint32_t selectorSteps = mm::unitToSteps<mm::S_pos_t>(config::selectorLimits.lenght);
    uint32_t maxSteps = std::max(idlerSteps, selectorSteps) + 1;

    for (uint32_t i = 0; i < maxSteps; ++i) {
        main_loop();
        cb.Step();

        if (i == idlerSteps) {
            mm::TriggerStallGuard(mm::Idler);
        } else {
            mm::motion.StallGuardReset(mm::Idler);
        }
        if (i == selectorSteps) {
            mm::TriggerStallGuard(mm::Selector);
        } else {
            mm::motion.StallGuardReset(mm::Selector);
        }
    }

    // now the Selector and Idler shall perform a move into their parking positions
    while (ms::selector.State() != mm::MovableBase::Ready || mi::idler.State() != mm::MovableBase::Ready) {
        main_loop();
        cb.Step();
    }
#else
    // sadly, it looks like we need to separate homing of idler and selector due to electrical reasons
    SimulateIdlerHoming(cb);
    SimulateSelectorHoming(cb);
#endif
}

void SimulateIdlerHoming(logic::CommandBase &cb) {
    // do 5 steps until we trigger the simulated StallGuard
    for (uint8_t i = 0; i < 5; ++i) {
        main_loop();
        cb.Step();
    }

    mm::TriggerStallGuard(mm::Idler);
    main_loop();
    cb.Step();
    mm::motion.StallGuardReset(mm::Idler);

    // now do a correct amount of steps of each axis towards the other end
    uint32_t idlerSteps = mm::unitToSteps<mm::I_pos_t>(config::idlerLimits.lenght);
    uint32_t maxSteps = idlerSteps + 1;

    for (uint32_t i = 0; i < maxSteps; ++i) {
        main_loop();
        cb.Step();

        if (i == idlerSteps) {
            mm::TriggerStallGuard(mm::Idler);
        } else {
            mm::motion.StallGuardReset(mm::Idler);
        }
    }

    // now the Idler shall perform a move into their parking positions
    while (mi::idler.State() != mm::MovableBase::Ready) {
        main_loop();
        cb.Step();
    }
}

void SimulateSelectorHoming(logic::CommandBase &cb) {
    // do 5 steps until we trigger the simulated StallGuard
    for (uint8_t i = 0; i < 5; ++i) {
        main_loop();
        cb.Step();
    }

    mm::TriggerStallGuard(mm::Selector);
    main_loop();
    cb.Step();
    mm::motion.StallGuardReset(mm::Selector);

    // now do a correct amount of steps of each axis towards the other end
    uint32_t selectorSteps = mm::unitToSteps<mm::S_pos_t>(config::selectorLimits.lenght) + 1;
    uint32_t maxSteps = selectorSteps + 1;

    for (uint32_t i = 0; i < maxSteps; ++i) {
        main_loop();
        cb.Step();

        if (i == selectorSteps) {
            mm::TriggerStallGuard(mm::Selector);
        } else {
            mm::motion.StallGuardReset(mm::Selector);
        }
    }

    // now the Selector shall perform a move into their parking positions
    while (ms::selector.State() != mm::MovableBase::Ready) {
        main_loop();
        cb.Step();
    }
}

bool SimulateFailedHomeSelectorPostfix(logic::CommandBase &cb) {
    if (!WhileTopState(cb, ProgressCode::Homing, 5))
        return false;
    if (cb.Error() != ErrorCode::HOMING_SELECTOR_FAILED)
        return false;
    if (cb.State() != ProgressCode::ERRWaitingForUser)
        return false;
    if (mm::motion.Enabled(mm::Selector))
        return false;

    // do a few steps before pushing the button
    WhileTopState(cb, ProgressCode::ERRWaitingForUser, 5);

    if (mm::motion.Enabled(mm::Selector))
        return false;

    PressButtonAndDebounce(cb, mb::Middle, false);

    // it shall start homing again
    if (cb.Error() != ErrorCode::RUNNING)
        return false;
    if (cb.State() != ProgressCode::Homing)
        return false;
    if (ms::selector.HomingValid())
        return false;
    if (!mm::motion.Enabled(mm::Selector))
        return false;

    ClearButtons(cb);

    return true;
}

bool SimulateFailedHomeFirstTime(logic::CommandBase &cb) {
    if (mi::idler.HomingValid())
        return false;
    if (ms::selector.HomingValid())
        return false;

    {
        // do 5 steps until we trigger the simulated StallGuard
        for (uint8_t i = 0; i < 5; ++i) {
            main_loop();
            cb.Step();
        }

        mm::TriggerStallGuard(mm::Selector);
        mm::TriggerStallGuard(mm::Idler);
        main_loop();
        cb.Step();
        mm::motion.StallGuardReset(mm::Selector);
        mm::motion.StallGuardReset(mm::Idler);
    }
    // now do a correct amount of steps of each axis towards the other end
    uint32_t idlerSteps = mm::unitToSteps<mm::I_pos_t>(config::idlerLimits.lenght);
    // now do LESS steps than expected to simulate something is blocking the selector
    uint32_t selectorSteps = mm::unitToSteps<mm::S_pos_t>(config::selectorLimits.lenght) + 1;
    uint32_t selectorTriggerShort = std::min(idlerSteps, selectorSteps) / 2;
    uint32_t maxSteps = selectorTriggerShort + 1;
    {
        for (uint32_t i = 0; i < maxSteps; ++i) {
            main_loop();
            cb.Step();

            if (i == selectorTriggerShort) {
                mm::TriggerStallGuard(mm::Selector);
            } else {
                mm::motion.StallGuardReset(mm::Selector);
            }
        }

        // make sure the Idler finishes its homing procedure (makes further checks much easier)
        for (uint32_t i = maxSteps; i < idlerSteps + 1; ++i) {
            main_loop();
            cb.Step();
            if (i == idlerSteps) {
                mm::TriggerStallGuard(mm::Idler);
            } else {
                mm::motion.StallGuardReset(mm::Idler);
            }
        }

        while (ms::selector.State() != mm::MovableBase::HomingFailed) {
            main_loop();
            cb.Step();
        }
    }

    return SimulateFailedHomeSelectorPostfix(cb);
}

bool SimulateFailedHomeSelectorRepeated(logic::CommandBase &cb) {
    // we leave Idler aside in this case
    if (ms::selector.HomingValid())
        return false;

    {
        // do 5 steps until we trigger the simulated StallGuard
        for (uint8_t i = 0; i < 5; ++i) {
            main_loop();
            cb.Step();
        }

        mm::TriggerStallGuard(mm::Selector);
        main_loop();
        cb.Step();
        mm::motion.StallGuardReset(mm::Selector);
    }
    uint32_t selectorSteps = mm::unitToSteps<mm::S_pos_t>(config::selectorLimits.lenght) + 1;
    uint32_t selectorTriggerShort = selectorSteps / 2;
    uint32_t maxSteps = selectorTriggerShort + 1;
    {
        for (uint32_t i = 0; i < maxSteps; ++i) {
            main_loop();
            cb.Step();

            if (i == selectorTriggerShort) {
                mm::TriggerStallGuard(mm::Selector);
            } else {
                mm::motion.StallGuardReset(mm::Selector);
            }
        }

        while (ms::selector.State() != mm::MovableBase::HomingFailed) {
            main_loop();
            cb.Step();
        }
    }

    return SimulateFailedHomeSelectorPostfix(cb);
}
