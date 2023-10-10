// Software implementation of random.h

#include "random.h"

#include <stdlib.h>

RAND_DECL uint32_t rand_u() {
    return uint32_t(std::rand());
}
