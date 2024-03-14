#include <mmu2_fsensor.h>

namespace MMU2 {

FilamentState fs = FilamentState::NOT_PRESENT;

FilamentState WhereIsFilament() {
    return fs;
}

FSensorBlockRunout::FSensorBlockRunout() {}
FSensorBlockRunout::~FSensorBlockRunout() {}

} // namespace MMU2
