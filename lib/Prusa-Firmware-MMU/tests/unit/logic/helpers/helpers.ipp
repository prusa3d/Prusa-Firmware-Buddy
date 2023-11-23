bool VerifyEnvironmentState(mg::FilamentLoadState fls, uint8_t idlerSlotIndex, uint8_t selectorSlotIndex,
    bool findaPressed, bool pulleyEnabled, ml::Mode greenLEDMode, ml::Mode redLEDMode) {
    CHECKED_ELSE(mg::globals.FilamentLoaded() & fls) { // beware - abusing the values as bit masks to detect multiple situations at once
        WARN("FilamentLoaded mismatch");
        WARN("mg::globals.FilamentLoaded()): " << (int)mg::globals.FilamentLoaded());
        WARN("Expected state, fls: " << (int)fls);
        return false;
    }
    if( mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector ){
        // check eeprom content - filament blocking the selector
        uint8_t eeSlot = 0xff;
        CHECKED_ELSE(mps::FilamentLoaded::get(eeSlot)){
            WARN("mps::FilamentLoaded::get(eeSlot) returned false");
            return false;
        }
        CHECKED_ELSE(eeSlot == selectorSlotIndex){
            WARN("eeSlot does not match selectorSlotIndex");
            WARN("eeSlot: " << (int)eeSlot);
            WARN("selectorSlotIndex: " << (int)selectorSlotIndex);
            return false;
        }
    } else {
        // check eeprom content - filament blocking the selector
        uint8_t eeSlot = 0xff;
        CHECKED_ELSE(mps::FilamentLoaded::get(eeSlot) == false){
            WARN("mps::FilamentLoaded::get(eeSlot) returned true");
            return false;
        }
    }

    if( idlerSlotIndex < config::toolCount ){ // abusing invalid index to skip checking of slot and position
        CHECKED_ELSE(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(idlerSlotIndex).v) {
            WARN("mm::axes[mm::Idler].pos != mi::Idler::SlotPosition(idlerSlotIndex).v");
            return false;
        }
        CHECKED_ELSE(mi::idler.Engaged() == (idlerSlotIndex < config::toolCount)) {
            WARN("mi::idler.Engaged() != (idlerSlotIndex < config::toolCount)");
            return false;
        }
    }
    if( mi::idler.State() == mi::Idler::HomeForward || mi::idler.State() == mi::Idler::HomeBack ){
        CHECKED_ELSE(mi::idler.Slot() == 0xff){
            WARN("mi::idler.Slot() != 0xff");
            return false;
        }
        CHECKED_ELSE(mi::idler.HomingValid() == false){
            WARN("mi::idler.HomingValid() != false");
            return false;
        }
    }

    if( selectorSlotIndex < config::toolCount ){ // abusing invalid index to skip checking of slot and position
        CHECKED_ELSE(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(selectorSlotIndex).v) {
            WARN("mm::axes[mm::Selector].pos != ms::Selector::SlotPosition(selectorSlotIndex).v");
            return false;
        }
        CHECKED_ELSE(ms::selector.Slot() == selectorSlotIndex) {
            WARN("ms::selector.Slot() != selectorSlotIndex");
            return false;
        }
    }
    if( ms::selector.State() == ms::Selector::HomeForward || ms::selector.State() == ms::Selector::HomeBack ){
        CHECKED_ELSE(ms::selector.Slot() == 0xff){
            WARN("ms::selector.Slot() != 0xff");
            return false;
        }
        CHECKED_ELSE(ms::selector.HomingValid() == false){
            WARN("ms::selector.Slot() != 0xff");
            return false;
        }
    }

    CHECKED_ELSE(mf::finda.Pressed() == findaPressed) {
        if(mf::finda.Pressed() && !findaPressed) WARN("FINDA is expected to be 0, but its 1");
        else WARN("FINDA is expected to be 1, but its 0");
        return false;
    }
    CHECKED_ELSE(mm::PulleyEnabled() == pulleyEnabled){
        if(mm::PulleyEnabled() && !pulleyEnabled) WARN("Pulley motor is expected to be disabled, but its enabled");
        else WARN("Pulley motor is expected to be enabled, but its disabled");
        return false;
    }

    for(uint8_t ledIndex = 0; ledIndex < config::toolCount; ++ledIndex){
        if( ledIndex != selectorSlotIndex ){
            // the other LEDs should be off
            auto red = ml::leds.Mode(ledIndex, ml::red);
            CHECKED_ELSE(red == ml::off) {
                WARN("red != ml::off");
                return false;
            }
            auto green = ml::leds.Mode(ledIndex, ml::green);
            CHECKED_ELSE(green == ml::off) {
                WARN("green != ml::off");
                return false;
            }
        } else {
            auto red = ml::leds.Mode(selectorSlotIndex, ml::red);
            CHECKED_ELSE(red == redLEDMode) {
                WARN("red != redLEDMode");
                return false;
            }
            auto green = ml::leds.Mode(selectorSlotIndex, ml::green);
            CHECKED_ELSE(green == greenLEDMode) {
                WARN("green != greenLEDMode");
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

    CHECKED_ELSE(uf.Error() == err) {
        WARN("uf.Error() != err");
        WARN("Expecting error code, err: " << (int)err);
        WARN("But actual error code is, uf.Error(): " << (int)uf.Error());
        return false;
    }
    auto tls = uf.TopLevelState();
    CHECKED_ELSE(tls == topLevelProgress) {
        WARN("tls != topLevelProgress");
        return false;
    }

    CHECKED_ELSE(VerifyEnvironmentState(fls, idlerSlotIndex, selectorSlotIndex, findaPressed, pulleyEnabled, greenLEDMode, redLEDMode)) {
        return false;
    }

    return true;
}

// LED checked at their own ledCheckIndex index
template<typename SM>
bool VerifyState2(SM &uf, mg::FilamentLoadState fls, uint8_t idlerSlotIndex, uint8_t selectorSlotIndex,
    bool findaPressed, bool pulleyEnabled, uint8_t ledCheckIndex, ml::Mode greenLEDMode, ml::Mode redLEDMode, ErrorCode err, ProgressCode topLevelProgress) {
    CHECKED_ELSE(mg::globals.FilamentLoaded() & fls) {
        WARN("mg::globals.FilamentLoaded() & fls is false");
        return false;
    }
    if( idlerSlotIndex < config::toolCount ){ // abusing invalid index to skip checking of slot and position
        CHECKED_ELSE(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(idlerSlotIndex).v) {
            WARN("mm::axes[mm::Idler].pos != mi::Idler::SlotPosition(idlerSlotIndex).v");
            return false;
        }
        CHECKED_ELSE(mi::idler.Engaged() == (idlerSlotIndex < config::toolCount)) {
            WARN("mi::idler.Engaged() != (idlerSlotIndex < config::toolCount)");
            return false;
        }
    }
    if( selectorSlotIndex < config::toolCount ){ // abusing invalid index to skip checking of slot and position
        CHECKED_ELSE(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(selectorSlotIndex).v) {
            WARN("mm::axes[mm::Selector].pos != ms::Selector::SlotPosition(selectorSlotIndex).v");
            return false;
        }
        CHECKED_ELSE(ms::selector.Slot() == selectorSlotIndex) {
            WARN("ms::selector.Slot() != selectorSlotIndex");
            return false;
        }
    }
    CHECKED_ELSE(mf::finda.Pressed() == findaPressed) {
        WARN("mf::finda.Pressed() != findaPressed");
        return false;
    }
    CHECKED_ELSE(mm::PulleyEnabled() == pulleyEnabled) {
        WARN("mm::PulleyEnabled() != pulleyEnabled");
        return false;
    }

    for(uint8_t ledIndex = 0; ledIndex < config::toolCount; ++ledIndex){
        if( ledIndex != ledCheckIndex ){
            // the other LEDs should be off
            CHECKED_ELSE(ml::leds.Mode(ledIndex, ml::red) == ml::off) {
                WARN("ml::leds.Mode(ledIndex, ml::red) != ml::off");
                return false;
            }
            CHECKED_ELSE(ml::leds.Mode(ledIndex, ml::green) == ml::off) {
                WARN("ml::leds.Mode(ledIndex, ml::green) != ml::off");
                return false;
            }
        } else {
            auto lmr = ml::leds.Mode(ledCheckIndex, ml::red);
            CHECKED_ELSE(lmr == redLEDMode) {
                WARN("lmr != redLEDMode");
                return false;
            }
            auto lmg = ml::leds.Mode(ledCheckIndex, ml::green);
            CHECKED_ELSE(lmg == greenLEDMode) {
                WARN("lmg != greenLEDMode");
                return false;
            }
        }
    }

    CHECKED_ELSE(uf.Error() == err) {
        WARN("uf.Error() != err");
        WARN("Expecting error code, err: " << (int)err);
        WARN("But actual error code is, uf.Error(): " << (int)uf.Error());
        return false;
    }
    CHECKED_ELSE(uf.TopLevelState() == topLevelProgress) {
        WARN("uf.TopLevelState() != topLevelProgress");
        WARN("Expecting progress code, topLevelProgress: " << (int)topLevelProgress);
        WARN("But actual error code is, uf.TopLevelState(): " << (int)uf.TopLevelState());
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
