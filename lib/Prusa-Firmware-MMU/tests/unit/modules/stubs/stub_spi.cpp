#include "spi.h"

namespace hal {
namespace spi {

void Init(SPI_TypeDef *const hspi, SPI_InitTypeDef *const conf) {
}

uint8_t TxRx(SPI_TypeDef *hspi, uint8_t val) {
    return 0;
}

} // namespace spi
} // namespace hal
