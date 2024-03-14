/// @file spi.cpp
#include "../spi.h"

namespace hal {
namespace spi {

#ifdef __AVR__
void Init(SPI_TypeDef *const hspi, SPI_InitTypeDef *const conf) {
    gpio::Init(conf->miso_pin, gpio::GPIO_InitTypeDef(gpio::Mode::input, gpio::Pull::none));
    gpio::Init(conf->mosi_pin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::low));
    gpio::Init(conf->sck_pin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::low));
    gpio::Init(conf->ss_pin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::high)); //the AVR requires this pin to be an output for SPI master mode to work properly.

    const uint8_t spi2x = (conf->prescaler == 7) ? 0 : (conf->prescaler & 0x01);
    const uint8_t spr = ((conf->prescaler - 1) >> 1) & 0x03;

    hspi->SPCRx = (0 << SPIE) | (1 << SPE) | (0 << DORD) | (1 << MSTR) | ((conf->cpol & 0x01) << CPOL) | ((conf->cpha & 0x01) << CPHA) | (spr << SPR0);
    hspi->SPSRx = (spi2x << SPI2X);
}

uint8_t TxRx(SPI_TypeDef *hspi, uint8_t val) {
    hspi->SPDRx = val;
    while (!(hspi->SPSRx & (1 << SPIF)))
        ;
    return hspi->SPDRx;
}
#endif

} // namespace spi
} // namespace hal
