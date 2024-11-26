#include "conserve_cpu.hpp"

namespace buddy {

ConserveCpu &conserve_cpu() {
    static ConserveCpu instance;
    return instance;
}

} // namespace buddy
