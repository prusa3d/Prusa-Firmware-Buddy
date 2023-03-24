bool VerifyEnvironmentState(mg::FilamentLoadState fls, uint8_t idlerSlotIndex, uint8_t selectorSlotIndex,
    bool findaPressed, bool pulleyEnabled, ml::Mode greenLEDMode, ml::Mode redLEDMode) {
    CHECKED_ELSE(mg::globals.FilamentLoaded() & fls) { // beware - abusing the values as bit masks to detect multiple situations at once
    return false;
    }
    if( mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector ){
        // check eeprom content - filament blocking the selector
        uint8_t eeSlot = 0xff;
        CHECKED_ELSE(mps::FilamentLoaded::get(eeSlot)){
        return false;
        }
        CHECKED_ELSE(eeSlot == selectorSlotIndex){
        return false;
        }
    } else {
        // check eeprom content - filament blocking the selector
        uint8_t eeSlot = 0xff;
        CHECKED_ELSE(mps::FilamentLoaded::get(eeSlot) == false){
        return false;
        }
    }
    CHECKED_ELSE(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(idlerSlotIndex).v) {
    return false;
    }
    CHECKED_ELSE(mi::idler.Engaged() == (idlerSlotIndex < config::toolCount)) {
    return false;
    }
    CHECKED_ELSE(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(selectorSlotIndex).v) {
    return false;
    }
    CHECKED_ELSE(ms::selector.Slot() == selectorSlotIndex) {
    return false;
    }
    CHECKED_ELSE(mf::finda.Pressed() == findaPressed) {
    return false;
    }
    CHECKED_ELSE(mm::PulleyEnabled() == pulleyEnabled){
    return false;
    }

    for(uint8_t ledIndex = 0; ledIndex < config::toolCount; ++ledIndex){
        if( ledIndex != selectorSlotIndex ){
            // the other LEDs should be off
            CHECKED_ELSE(ml::leds.Mode(ledIndex, ml::red) == ml::off) {
            return false;
            }
            CHECKED_ELSE(ml::leds.Mode(ledIndex, ml::green) == ml::off) {
            return false;
            }
        } else {
            CHECKED_ELSE(ml::leds.Mode(selectorSlotIndex, ml::red) == redLEDMode) {
            return false;
            }
            CHECKED_ELSE(ml::leds.Mode(selectorSlotIndex, ml::green) == greenLEDMode) {
            return false;
            }
        }
    }
    return true;
}

// LED checked at selector's index
template<typename SM>
bool VerifyState(SM &uf, mg::FilamentLoadState fls, uint8_t idlerSlotIndex, uint8_t selectorSlotIndex,
    bool findaPressed, bool pulleyEnabled, ml::Mode greenLEDMode, ml::Mode redLEDMode, ErrorCode err, ProgressCode topLevelProgress) {

    VerifyEnvironmentState(fls, idlerSlotIndex, selectorSlotIndex, findaPressed, pulleyEnabled, greenLEDMode, redLEDMode);

    CHECKED_ELSE(uf.Error() == err) {
        return false;
    }
    CHECKED_ELSE(uf.TopLevelState() == topLevelProgress) {
    return false;
    }
    return true;
}

// LED checked at their own ledCheckIndex index
template<typename SM>
bool VerifyState2(SM &uf, mg::FilamentLoadState fls, uint8_t idlerSlotIndex, uint8_t selectorSlotIndex,
    bool findaPressed, bool pulleyEnabled, uint8_t ledCheckIndex, ml::Mode greenLEDMode, ml::Mode redLEDMode, ErrorCode err, ProgressCode topLevelProgress) {
    CHECKED_ELSE(mg::globals.FilamentLoaded() & fls) {
    return false;
    }
    CHECKED_ELSE(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(idlerSlotIndex).v) {
    return false;
    }
    CHECKED_ELSE(mi::idler.Engaged() == (idlerSlotIndex < config::toolCount)) {
    return false;
    }
    CHECKED_ELSE(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(selectorSlotIndex).v) {
    return false;
    }
    CHECKED_ELSE(ms::selector.Slot() == selectorSlotIndex) {
    return false;
    }
    CHECKED_ELSE(mf::finda.Pressed() == findaPressed) {
    return false;
    }
    CHECKED_ELSE(mm::PulleyEnabled() == pulleyEnabled){
    return false;
    }

    for(uint8_t ledIndex = 0; ledIndex < config::toolCount; ++ledIndex){
        if( ledIndex != ledCheckIndex ){
            // the other LEDs should be off
            CHECKED_ELSE(ml::leds.Mode(ledIndex, ml::red) == ml::off) {
            return false;
            }
            CHECKED_ELSE(ml::leds.Mode(ledIndex, ml::green) == ml::off) {
            return false;
            }
        } else {
            CHECKED_ELSE(ml::leds.Mode(ledCheckIndex, ml::red) == redLEDMode) {
            return false;
            }
            CHECKED_ELSE(ml::leds.Mode(ledCheckIndex, ml::green) == greenLEDMode) {
            return false;
            }
        }
    }

    CHECKED_ELSE(uf.Error() == err) {
    return false;
    }
    CHECKED_ELSE(uf.TopLevelState() == topLevelProgress) {
    return false;
    }
    return true;
}



template<typename SM>
void InvalidSlot(SM &logicSM,  uint8_t activeSlot, uint8_t invSlot){
    ForceReinitAllAutomata();
    EnsureActiveSlotIndex(5, mg::FilamentLoadState::AtPulley);

    REQUIRE(VerifyEnvironmentState(mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), ms::Selector::IdleSlotIndex(), false, false, ml::off, ml::off));

    EnsureActiveSlotIndex(activeSlot, mg::FilamentLoadState::AtPulley);

    logicSM.Reset(invSlot);
    REQUIRE(VerifyState(logicSM, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), activeSlot, false, false, ml::off, ml::off, ErrorCode::INVALID_TOOL, ProgressCode::OK));
}

template <typename SM>
void PressButtonAndDebounce(SM &sm, uint8_t btnIndex){
    hal::adc::SetADC(config::buttonsADCIndex, config::buttonADCLimits[btnIndex][0] + 1);
    while (!mb::buttons.ButtonPressed(btnIndex)) {
        main_loop();
        sm.StepInner();
    }
}
