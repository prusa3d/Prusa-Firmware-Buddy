/**
 * @file preheat_multithread_status.hpp
 * @author Radek Vana
 * @brief Multi thread status os preheat, so GUI can synchronize dialogs
 * for now only 1 (other than this) thread is suported
 * @date 2021-01-23
 */
#pragma once

namespace PreheatStatus {
enum class Result {
    DidNotFinish,
    DoneHasFilament,
    DoneNoFilament,
    Aborted,
    Error,
    CooledDown
};

Result ConsumeResult(); // also used to clear

} // namespace PreheatStatus
