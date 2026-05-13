#include "adc_sense_drv.h"

#include "adc.h"
#include "stm32f4xx_hal.h"

#include <limits.h>
#include <stddef.h>

#define ADC_SENSE_ADC_MAX_COUNT    (4095U)
#define ADC_SENSE_VDDA_MV_NOMINAL  (3300U)
#define ADC_SENSE_LTC3901_DIVIDER_HIGH_OHMS (100000L)
#define ADC_SENSE_LTC3901_DIVIDER_LOW_OHMS  (37400L)

typedef struct
{
    int32_t SlopeScaled;
    int32_t Offset;
    bool Valid;
} adc_sense_calibration_entry_t;

static uint16_t s_adc_samples[ADC_SENSE_CHANNEL_COUNT];
static bool s_adc_sense_initialized;
static volatile bool s_adc_data_valid;
static volatile bool s_adc_dma_rearm_pending;
static adc_sense_calibration_entry_t s_adc_calibration[ADC_SENSE_CHANNEL_COUNT];

static bool adc_sense_drv_is_valid_channel(adc_sense_channel_t channel)
{
    return ((unsigned int)channel < (unsigned int)ADC_SENSE_CHANNEL_COUNT);
}

static int32_t adc_sense_drv_ltc3901_divider_slope_scaled(void)
{
    const int64_t divider_total_ohms =
        (int64_t)ADC_SENSE_LTC3901_DIVIDER_HIGH_OHMS +
        (int64_t)ADC_SENSE_LTC3901_DIVIDER_LOW_OHMS;
    const int64_t divisor =
        (int64_t)ADC_SENSE_ADC_MAX_COUNT *
        (int64_t)ADC_SENSE_LTC3901_DIVIDER_LOW_OHMS;
    const int64_t dividend =
        (int64_t)ADC_SENSE_VDDA_MV_NOMINAL *
        divider_total_ohms *
        (int64_t)ADC_SENSE_CALIBRATION_SLOPE_SCALE;

    return (int32_t)((dividend + (divisor / 2LL)) / divisor);
}

static int32_t adc_sense_drv_counts_to_millivolts(uint16_t counts)
{
    uint32_t scaled_mv;

    scaled_mv = ((uint32_t)counts * (uint32_t)ADC_SENSE_VDDA_MV_NOMINAL) / (uint32_t)ADC_SENSE_ADC_MAX_COUNT;
    return (int32_t)scaled_mv;
}

static void adc_sense_drv_load_default_calibration(void)
{
    size_t i;
    int32_t ltc3901_divider_slope_scaled;

    for (i = 0u; i < (size_t)ADC_SENSE_CHANNEL_COUNT; ++i)
    {
        s_adc_calibration[i].SlopeScaled = ADC_SENSE_CALIBRATION_SLOPE_SCALE;
        s_adc_calibration[i].Offset = 0L;
        s_adc_calibration[i].Valid = false;
    }

    ltc3901_divider_slope_scaled = adc_sense_drv_ltc3901_divider_slope_scaled();

    s_adc_calibration[ADC_SENSE_CHANNEL_VUPSTREAM].SlopeScaled =
        ltc3901_divider_slope_scaled;
    s_adc_calibration[ADC_SENSE_CHANNEL_VUPSTREAM].Offset = 0L;
    s_adc_calibration[ADC_SENSE_CHANNEL_VUPSTREAM].Valid = true;

    s_adc_calibration[ADC_SENSE_CHANNEL_LTC3901_VCC].SlopeScaled =
        ltc3901_divider_slope_scaled;
    s_adc_calibration[ADC_SENSE_CHANNEL_LTC3901_VCC].Offset = 0L;
    s_adc_calibration[ADC_SENSE_CHANNEL_LTC3901_VCC].Valid = true;
}

static bool adc_sense_drv_apply_calibration(uint16_t raw_counts,
                                             const adc_sense_calibration_entry_t *calibration,
                                             int32_t *value_out)
{
    int64_t scaled_value;

    if ((calibration == NULL) || (value_out == NULL) || !calibration->Valid)
    {
        return false;
    }

    scaled_value = ((int64_t)calibration->SlopeScaled * (int64_t)raw_counts) /
                   (int64_t)ADC_SENSE_CALIBRATION_SLOPE_SCALE;
    scaled_value += (int64_t)calibration->Offset;

    if ((scaled_value > (int64_t)INT32_MAX) || (scaled_value < (int64_t)INT32_MIN))
    {
        return false;
    }

    *value_out = (int32_t)scaled_value;
    return true;
}

static bool adc_sense_drv_start_dma(void)
{
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_adc_samples, (uint32_t)ADC_SENSE_CHANNEL_COUNT) == HAL_OK)
    {
        s_adc_sense_initialized = true;
        s_adc_dma_rearm_pending = false;
        return true;
    }

    s_adc_sense_initialized = false;
    s_adc_data_valid = false;
    s_adc_dma_rearm_pending = true;
    return false;
}

void adc_sense_drv_init(void)
{
    adc_sense_drv_load_default_calibration();
    (void)adc_sense_drv_start_dma();
}

void adc_sense_drv_task(void)
{
    if (!s_adc_dma_rearm_pending)
    {
        return;
    }

    (void)HAL_ADC_Stop_DMA(&hadc1);
    (void)adc_sense_drv_start_dma();
}

bool adc_sense_drv_is_initialized(void)
{
    return s_adc_sense_initialized;
}

bool adc_sense_drv_is_data_valid(void)
{
    return s_adc_data_valid;
}

uint16_t adc_sense_drv_get_raw(adc_sense_channel_t channel)
{
    if (!s_adc_sense_initialized || !s_adc_data_valid || !adc_sense_drv_is_valid_channel(channel))
    {
        return 0U;
    }

    return s_adc_samples[(unsigned int)channel];
}

int32_t adc_sense_drv_get_channel_millivolts(adc_sense_channel_t channel)
{
    if (!s_adc_sense_initialized || !s_adc_data_valid || !adc_sense_drv_is_valid_channel(channel))
    {
        return -1;
    }

    return adc_sense_drv_counts_to_millivolts(s_adc_samples[(unsigned int)channel]);
}

bool adc_sense_drv_get_calibration(adc_sense_channel_t channel, adc_sense_calibration_t *calibration_out)
{
    if (!adc_sense_drv_is_valid_channel(channel) || (calibration_out == NULL))
    {
        return false;
    }

    calibration_out->SlopeScaled = s_adc_calibration[(unsigned int)channel].SlopeScaled;
    calibration_out->Offset = s_adc_calibration[(unsigned int)channel].Offset;
    calibration_out->Valid = s_adc_calibration[(unsigned int)channel].Valid;
    return true;
}

bool adc_sense_drv_set_calibration(adc_sense_channel_t channel, const adc_sense_calibration_t *calibration)
{
    if (!adc_sense_drv_is_valid_channel(channel) || (calibration == NULL))
    {
        return false;
    }

    s_adc_calibration[(unsigned int)channel].SlopeScaled = calibration->SlopeScaled;
    s_adc_calibration[(unsigned int)channel].Offset = calibration->Offset;
    s_adc_calibration[(unsigned int)channel].Valid = calibration->Valid;
    return true;
}

void adc_sense_drv_clear_calibration(adc_sense_channel_t channel)
{
    if (!adc_sense_drv_is_valid_channel(channel))
    {
        return;
    }

    if ((channel == ADC_SENSE_CHANNEL_VUPSTREAM) ||
        (channel == ADC_SENSE_CHANNEL_LTC3901_VCC))
    {
        s_adc_calibration[(unsigned int)channel].SlopeScaled =
            adc_sense_drv_ltc3901_divider_slope_scaled();
        s_adc_calibration[(unsigned int)channel].Offset = 0L;
        s_adc_calibration[(unsigned int)channel].Valid = true;
        return;
    }

    s_adc_calibration[(unsigned int)channel].SlopeScaled = ADC_SENSE_CALIBRATION_SLOPE_SCALE;
    s_adc_calibration[(unsigned int)channel].Offset = 0L;
    s_adc_calibration[(unsigned int)channel].Valid = false;
}

bool adc_sense_drv_get_channel_engineering_units(adc_sense_channel_t channel, int32_t *value_out)
{
    if (!s_adc_sense_initialized || !s_adc_data_valid || !adc_sense_drv_is_valid_channel(channel))
    {
        return false;
    }

    return adc_sense_drv_apply_calibration(s_adc_samples[(unsigned int)channel],
                                           &s_adc_calibration[(unsigned int)channel],
                                           value_out);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1)
    {
        s_adc_data_valid = true;
        s_adc_dma_rearm_pending = true;
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1)
    {
        s_adc_data_valid = false;
        s_adc_dma_rearm_pending = true;
    }
}
