#ifndef ADC_SENSE_DRV_H
#define ADC_SENSE_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ADC_SENSE_CHANNEL_VUPSTREAM = 0,
    ADC_SENSE_CHANNEL_LTC3901_VCC,
    ADC_SENSE_CHANNEL_LT8316_VOUT,
    ADC_SENSE_CHANNEL_LTC3901_ME_ANLG,
    ADC_SENSE_CHANNEL_LTC3901_MF_ANLG,
    ADC_SENSE_CHANNEL_LT8316_GATE_ANLG,
    ADC_SENSE_CHANNEL_TEMP,
    ADC_SENSE_CHANNEL_VREFINT,
    ADC_SENSE_CHANNEL_COUNT
} adc_sense_channel_t;

typedef struct
{
    int32_t SlopeScaled;
    int32_t Offset;
    bool Valid;
} adc_sense_calibration_t;

#define ADC_SENSE_CALIBRATION_SLOPE_SCALE    (1000000L)

void adc_sense_drv_init(void);
bool adc_sense_drv_is_initialized(void);
bool adc_sense_drv_is_data_valid(void);
uint16_t adc_sense_drv_get_raw(adc_sense_channel_t channel);
int32_t adc_sense_drv_get_channel_millivolts(adc_sense_channel_t channel);
bool adc_sense_drv_get_calibration(adc_sense_channel_t channel, adc_sense_calibration_t *calibration_out);
bool adc_sense_drv_set_calibration(adc_sense_channel_t channel, const adc_sense_calibration_t *calibration);
void adc_sense_drv_clear_calibration(adc_sense_channel_t channel);
bool adc_sense_drv_get_channel_engineering_units(adc_sense_channel_t channel, int32_t *value_out);

#ifdef __cplusplus
}
#endif

#endif /* ADC_SENSE_DRV_H */
