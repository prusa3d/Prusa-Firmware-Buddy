///@file
#include "hal_clock.hpp"

#include <stm32h503xx.h>

// PLL parameters
static constexpr const uint32_t PLL1M = 12;
static constexpr const uint32_t PLL1N = 240;
static constexpr const uint32_t PLL1P = 2;
static constexpr const uint32_t PLL1Q = 10;
static constexpr const uint32_t PLL1R = 2;

// Clock frequencies.
// See Figure 31 in RM0492 for details.
static constexpr const uint32_t hse_ck = 24'000'000; // external crystal oscillator
static constexpr const uint32_t pll1_p_ck = ((hse_ck / PLL1M) * PLL1N) / PLL1P;
static constexpr const uint32_t pll1_q_ck = ((hse_ck / PLL1M) * PLL1N) / PLL1Q;
static constexpr const uint32_t pll1_r_ck = ((hse_ck / PLL1M) * PLL1N) / PLL1R;
static constexpr const uint32_t sys_ck = pll1_p_ck;
static_assert(pll1_q_ck == 48'000'000); // USB requires 48 MHz
static_assert(sys_ck <= 250'000'000); // System clock can't be higher than 250 MHz

static void configure_pwr() {
    // Increase voltage scaling in order to sustain higher clock frequency
    // and wait until power is stable.
    constexpr const uint32_t VOSCR = 0
        | (0b11 << PWR_VOSCR_VOS_Pos) // voltage scale 0
        ;
    PWR->VOSCR = VOSCR;
    while ((PWR->VOSSR & PWR_VOSSR_VOSRDY) == 0)
        ;
}

static void configure_hse() {
    // Enable HSE (high-speed external oscillator) and wait until it is stable.
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0)
        ;
}

static void configure_pll1() {
    // Configure PLL1 (phase-locked loop 1) as follows:
    //  input:   24 MHz HSE
    //  output: 240 MHz SYSCLK (24 MHz / 12 * 240 /  2)
    //           48 MHz USB    (24 MHz / 12 * 240 / 10)
    // See Figure 36 in RM0492 for details.
    constexpr const uint32_t PLL1CFGR = 0
        | (0b11 << RCC_PLL1CFGR_PLL1SRC_Pos) // HSE selected as PLL clock
        | (0b01 << RCC_PLL1CFGR_PLL1RGE_Pos) // input clock range frequency (after dividing by PLL1M) between 2 and 4 MHz
        | (0b0 << RCC_PLL1CFGR_PLL1FRACEN_Pos) // disable fractional latch
        | (0b0 << RCC_PLL1CFGR_PLL1VCOSEL_Pos) // wide VCO range 192 to 836 MHz
        | (PLL1M << RCC_PLL1CFGR_PLL1M_Pos) // prescaler
        | (0b1 << RCC_PLL1CFGR_PLL1PEN_Pos) // enable DIVP divider output
        | (0b1 << RCC_PLL1CFGR_PLL1QEN_Pos) // enable DIVQ divider output
        | (0b0 << RCC_PLL1CFGR_PLL1REN_Pos) // disable DIVR divider output
        ;
    constexpr const uint32_t PLL1DIVR = 0
        | ((PLL1N - 1UL) << RCC_PLL1DIVR_PLL1N_Pos) // multiplication factor for VCO
        | ((PLL1P - 1UL) << RCC_PLL1DIVR_PLL1P_Pos) // DIVP division factor
        | ((PLL1Q - 1UL) << RCC_PLL1DIVR_PLL1Q_Pos) // DIVQ division factor
        | ((PLL1R - 1UL) << RCC_PLL1DIVR_PLL1R_Pos) // DIVR division factor
        ;
    RCC->PLL1CFGR = PLL1CFGR;
    RCC->PLL1DIVR = PLL1DIVR;

    // Enable PLL1 and wait until it is stable.
    RCC->CR |= RCC_CR_PLL1ON;
    while ((RCC->CR & RCC_CR_PLL1RDY) == 0)
        ;
}

static void configure_flash() {
    // At higher clock frequencies and voltages, FLASH memory needs
    // to be configured to wait additional cycles when reading or writing.
    // Since we are waiting so much, we are allowed to enable prefetch too.
    // See Table 20 in RM0492 for details.
    constexpr const uint32_t ACR = 0
        | (0b1 << FLASH_ACR_PRFTEN_Pos) // enable prefetch
        | (5 << FLASH_ACR_LATENCY_Pos) // read latency
        | (0b10 << FLASH_ACR_WRHIGHFREQ_Pos) // flash signal delay
        ;
    FLASH->ACR = ACR;
    while (FLASH->ACR != ACR)
        ;
}

static void configure_cpu_domain_clock() {
    // Configure additional prescalers as follows:
    //  APB1:    30 MHz (240MHz / 8)
    //  APB2:    30 MHz (240MHz / 8)
    //  APB3:    30 MHz (240MHz / 8)
    //  SysCLK: 240 MHz (240MHz / 1)
    constexpr const uint32_t CFGR2 = 0
        | (0b0000 << RCC_CFGR2_HPRE_Pos) // AHB prescaler div1
        | (0b110 << RCC_CFGR2_PPRE1_Pos) // APB1 prescaler div8
        | (0b110 << RCC_CFGR2_PPRE2_Pos) // APB2 prescaler div8
        | (0b110 << RCC_CFGR2_PPRE3_Pos) // APB3 prescaler div8
        | (0b0 << RCC_CFGR2_AHB1DIS_Pos) // enable AHB1 clock
        | (0b0 << RCC_CFGR2_AHB2DIS_Pos) // enable AHB2 clock
        | (0b0 << RCC_CFGR2_APB1DIS_Pos) // enable APB1 clock
        | (0b0 << RCC_CFGR2_APB2DIS_Pos) // enable APB2 clock
        | (0b0 << RCC_CFGR2_APB3DIS_Pos) // enable APB3 clock
        ;
    RCC->CFGR2 = CFGR2;
}

static void switch_clock() {
    // Switch system clock source to PLL1 and wait for the switch to take place.
    RCC->CFGR1 = (RCC->CFGR1 & ~RCC_CFGR1_SW_Msk) | (0b11 << RCC_CFGR1_SW_Pos);
    while ((RCC->CFGR1 & RCC_CFGR1_SWS_Msk) != (0b11 << RCC_CFGR1_SWS_Pos))
        ;
}

// Beware: Before calling this function, make sure HSE and PLL1 are disabled!
void hal::overclock() {
    configure_pwr();
    configure_hse();
    configure_pll1();
    configure_flash();
    configure_cpu_domain_clock();
    switch_clock();

    // Set a variable for FreeRTOS subsystem. This could probably be more elegant...
    SystemCoreClock = sys_ck;
}
