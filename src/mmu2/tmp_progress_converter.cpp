#include "../../lib/Prusa-Firmware-MMU/src/logic/progress_codes.h"
#include <string.h>

namespace MMU2 {

void TranslateProgress(uint16_t pc, char *dst, size_t dstSize) {
    strlcat(dst, " ", dstSize);

    switch (pc) {
    case (uint16_t)ProgressCode::OK: {
        static const char s[] = "OK";
        strlcat(dst, s, dstSize);
    }
        return;

    case (uint16_t)ProgressCode::EngagingIdler: {
        static const char s[] = "EngIdler";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::DisengagingIdler: {
        static const char s[] = "DisengIdler";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::UnloadingToFinda: {
        static const char s[] = "Unl2Finda";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::UnloadingToPulley: {
        static const char s[] = "Unl2Pulley";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::FeedingToFinda: {
        static const char s[] = "Feed2Finda";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::FeedingToBondtech: {
        static const char s[] = "Feed2James";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::AvoidingGrind: {
        static const char s[] = "AvoidGrind";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::FinishingMoves: {
        static const char s[] = "FinMoves";
        strlcat(dst, s, dstSize);
    }
        return;

    case (uint16_t)ProgressCode::ERRDisengagingIdler: {
        static const char s[] = "ERRDisengIdler";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ERREngagingIdler: {
        static const char s[] = "ERREngIdler";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ERRWaitingForUser: {
        static const char s[] = "ERRWait4User";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ERRInternal: {
        static const char s[] = "ERRInternal";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ERRHelpingFilament: {
        static const char s[] = "ERRHelpFil";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ERRTMCFailed: {
        static const char s[] = "ERRTMCFail";
        strlcat(dst, s, dstSize);
    }
        return;

    case (uint16_t)ProgressCode::UnloadingFilament: {
        static const char s[] = "UnlFil";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::LoadingFilament: {
        static const char s[] = "LoadFil";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::SelectingFilamentSlot: {
        static const char s[] = "SelSlot";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::PreparingBlade: {
        static const char s[] = "PrepBlade";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::PushingFilament: {
        static const char s[] = "PushFil";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::PerformingCut: {
        static const char s[] = "PerfCut";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ReturningSelector: {
        static const char s[] = "RetSel";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::ParkingSelector: {
        static const char s[] = "ParkSel";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ProgressCode::EjectingFilament: {
        static const char s[] = "EjectFil";
        strlcat(dst, s, dstSize);
    }
        return;
    default: {
        static const char s[] = "ProgressUnkn";
        strlcat(dst, s, dstSize);
    }
        return;
    }
}

} // namespace MMU2
