#include <bsod/bsod.h>

#if defined(__cpp_contracts) && __cpp_contracts >= 202502L
    #include <contracts>
void handle_contract_violation(const std::contracts::contract_violation &violation) {
    _bsod("CONTRACT %s", violation.location().file_name(), violation.location().line(), violation.comment());
}
#endif
