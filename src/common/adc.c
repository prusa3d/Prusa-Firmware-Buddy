//adc.c
#include "adc.h"
#include "stm32f4xx_hal.h"
#include "hwio.h"
#include "main.h"

#ifndef ADC_SIM_MSK
    #define ADC_SIM_MSK 0
#endif

extern ADC_HandleTypeDef hadc1;

uint32_t adc_val[ADC_CHAN_CNT]; //sampled values
uint8_t adc_cnt[ADC_CHAN_CNT];  //number of samples
uint8_t adc_chn[ADC_CHAN_CNT];  //physical channels
uint8_t adc_sta = 0xff;         //current state, 0xff means "not initialized"
int8_t adc_idx = 0;             //current value index

uint32_t adc_sim_val[ADC_CHAN_CNT]; //simulated values
uint32_t adc_sim_msk = ADC_SIM_MSK; //mask simulated channels

void adc_init_sim_vals(void);

//convert value index to physical channel
uint8_t adc_chan(uint8_t idx) {
    uint8_t chan = 0;
    uint16_t mask = 1;
    while (mask) {
        if ((mask & ADC_CHAN_MSK) && (idx-- == 0))
            break;
        mask <<= 1;
        chan++;
    }
    return chan;
}

//configure multiplexer
void adc_set_mux(uint8_t chn) {
    ADC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.Channel = chn;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

//initialization
void adc_init(void) {
    for (int i = 0; i < ADC_CHAN_CNT; i++) {
        adc_val[i] = 0;
        adc_cnt[i] = 0;
        adc_chn[i] = adc_chan(i);
    }
    adc_idx = 0;
    adc_sta = 0x00;
    adc_init_sim_vals();
}

//sampling cycle (called from interrupt)
void adc_cycle(void) {
    uint8_t seq;
    uint32_t val;
    if (adc_sta == 0xff)
        return; //skip sampling if not initialized
    if (adc_sta & 0x80) {
        if (__HAL_ADC_GET_FLAG(&hadc1, ADC_FLAG_EOC)) {
            if ((1 << adc_idx) & adc_sim_msk)
                val = adc_sim_val[adc_idx];
            else
                val = HAL_ADC_GetValue(&hadc1);
            adc_val[adc_idx] += val;
            if (++adc_cnt[adc_idx] >= ADC_OVRSAMPL) {
                ADC_READY(adc_idx);
                adc_val[adc_idx] = 0;
                adc_cnt[adc_idx] = 0;
            }
            seq = (adc_sta & 0x1f) + 1;
            if (seq >= ADC_SEQ_LEN)
                seq = 0;
            adc_idx = ADC_SEQ2IDX(seq);
            adc_set_mux(adc_chn[adc_idx]);
            adc_sta = seq;
        } else {
            Error_Handler();
        }
    } else {
        HAL_ADC_Start(&hadc1);
        adc_sta |= 0x80;
    }
}

void adc_init_sim_vals(void) {
#ifdef ADC_SIM_VAL0
    adc_sim_val[0] = ADC_SIM_VAL0;
#endif //ADC_SIM_VAL0
#ifdef ADC_SIM_VAL1
    adc_sim_val[1] = ADC_SIM_VAL1;
#endif //ADC_SIM_VAL1
#ifdef ADC_SIM_VAL2
    adc_sim_val[2] = ADC_SIM_VAL2;
#endif //ADC_SIM_VAL2
#ifdef ADC_SIM_VAL3
    adc_sim_val[3] = ADC_SIM_VAL3;
#endif //ADC_SIM_VAL3
#ifdef ADC_SIM_VAL4
    adc_sim_val[4] = ADC_SIM_VAL4;
#endif //ADC_SIM_VAL4
#ifdef ADC_SIM_VAL5
    adc_sim_val[5] = ADC_SIM_VAL5;
#endif //ADC_SIM_VAL4
}
